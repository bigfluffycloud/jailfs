/*
 * tk/servers/jailfs:
 *	Package filesystem. Allows mounting various package files as
 *   a read-only (optionally with spillover file) overlay filesystem
 *   via FUSE or (eventually) LD_PRELOAD library.
 *
 * Copyright (C) 2012-2018 BigFluffy.Cloud <joseph@bigfluffy.cloud>
 *
 * Distributed under a MIT license. Send bugs/patches by email or
 * on github - https://github.com/bigfluffycloud/jailfs/
 *
 * No warranty of any kind. Good luck!
 * This file is ugly. It needs cleaned up and probably rewritten.
 *
 * Things are a bit intense in here as we try to do a bit of
 * caching of packages.
 *	* File is accessed causing package to be opened
 *		* ref count is increased (initially to 1)
 *	* File is closed
 *		* ref count is reduced
 *	* Another file in package is opened before the
 *	  garbage collect time out
 *		* Initial handle to package is reused
 *		* ref count increased
 *	....
 * This should help with things like compiling using several
 * headers from a package or starting programs which read
 * a bunch of config files.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <archive.h>
#include <archive_entry.h>
#include <lsd.h>
#include "conf.h"
#include "database.h"
#include "cron.h"
#include "shell.h"
#include "pkg.h"

/* This seems to be a BSD thing- it's not fatal if missing, so stub it */
#if	!defined(MAP_NOSYNC)
#define	MAP_NOSYNC	0
#endif                                 /* !defined(MAP_NOSYNC) */

// private module-global stuff 
BlockHeap *pkg_heap = NULL;            	// BlockHeap for packages
BlockHeap *pkg_file_heap = NULL;	// BlockHeap for package files
static dlink_list pkg_list;            	// List of currently opened packages
static time_t pkg_lifetime = 0;        	// see pkg_init() for initialization

static dlink_node *pkg_findnode(struct pkg_handle *pkg) {
   dlink_node *ptr, *tptr;

   DLINK_FOREACH_SAFE(ptr, tptr, pkg_list.head) {
      if ((struct pkg_handle *)ptr->data == pkg)
         return ptr;
   }

   return NULL;
}

/*
 * release the package handle
 *
 * Should only be called by the Garbage Collector
 * unless we get tight on memory (XXX: Add this)
 */
static void pkg_release(struct pkg_handle *pkg) {
   dlink_node *ptr;

   // XXX: Purge files
   // XXX: - Free file contents cache
   // XXX: - Free all pointers/structs associated

   // If name is allocated, free it 
   if (pkg->name) {
      mem_free(pkg->name);
      pkg->name = NULL;
   }

   // Unlock the file 
   flock(pkg->fd, LOCK_UN);

   // close handle, if exists 
   if (pkg->fd) {
      close(pkg->fd);
      pkg->fd = -1;
   }

   if ((ptr = pkg_findnode((struct pkg_handle *)pkg)) != NULL) {
      dlink_delete(ptr, &pkg_list);
      blockheap_free(pkg_heap, ptr->data);
   }
}

/*
 * Find a package by name
 * Used by pkg_open to locate an existing reference
 * to a package that hasn't been garbage collected yet
 */
struct pkg_handle *pkg_handle_byname(const char *path) {
   dlink_node *ptr, *tptr;
   struct pkg_handle *p;

   DLINK_FOREACH_SAFE(ptr, tptr, pkg_list.head) {
      p = (struct pkg_handle *)ptr->data;

      if (strcmp(p->name, path) == 0)
         return p;
   }

   return NULL;
}

/*
 * Scan for packages with refcnt == 0 every once in a while and close them
 * 	This helps to reduce closing and reopening packages unneededly
 *	Adjust tuning parameters in jailfs.cf as needed, based on your memory model.
 * if rfcnt == 0 && now > otime + (tuning.time.pkg_gc): release it
 */
static void pkg_gc(int fd, short event, void *arg) {
   dlink_node *ptr, *tptr;
   struct pkg_handle *p;

   DLINK_FOREACH_SAFE(ptr, tptr, pkg_list.head) {
      p = (struct pkg_handle *)ptr->data;

      if (p->refcnt == 0 && (time(NULL) > p->otime + pkg_lifetime))
         pkg_release(p);
   }
}

/*
 * Open a package, so that we can map files within it
 *
 * First, we try to see if the package already exists
 * in the cache. If it does, use it.
 *
 * Either way, we bump the ref count
 */
struct pkg_handle *pkg_open(const char *path) {
   struct pkg_handle *t;
   int         res = 0;

   // try to find an existing handle for the package
   // If this fails, create one and cache it...
   if ((t = pkg_handle_byname(path)) == NULL) {
      t = blockheap_alloc(pkg_heap);
      t->name = str_dup(path);

      if ((t->fd = open(t->name, O_RDONLY)) < 0) {
         Log(LOG_ERR, "failed opening pkg %s, bailing...", t->name);
         pkg_release(t);
         return NULL;
      }

      // Try to acquire an exclusive lock, fail if we cant 
      if (flock(t->fd, LOCK_EX | LOCK_NB) == -1) {
         Log(LOG_ERR, "failed locking package %s, bailing...", t->name);
         pkg_release(t);
         return NULL;
      }

      // Add handle to the cache list 
      dlink_add_tail_alloc(t, &pkg_list);
   }

   // Adjust last used time and reference count either way... 
   t->otime = time(NULL);
   t->refcnt++;

   return t;
}

/*
 * reduce the package's reference count
 *
 * We no longer free the package as the garbage collector
 * will do this for us after a delay, helping to keep packages
 * cached for a bit and reduce IO overhead
 */
void pkg_close(struct pkg_handle *pkg) {
   pkg->refcnt--;
   Log(LOG_DEBUG, "pkg_close: pkg %s refcnt == %d", pkg->name, pkg->refcnt);
}

/* Handle vfs_watch REMOVED event */
int pkg_forget(const char *path) {
   db_pkg_remove(path);
   return EXIT_SUCCESS;
}


void pkg_unmap_file(struct pkg_file_mapping *p) {
   if (p->addr != NULL && p->addr != MAP_FAILED)
      munmap(p->addr, p->len);

   if (p->pkg != NULL)
      mem_free(p->pkg);

   if (p->fd > 0)
      close(p->fd);

   blockheap_free(pkg_file_heap, p);
   p = NULL;
}

struct pkg_file_mapping *pkg_map_file(const char *path, size_t len, off_t offset) {
   struct pkg_file_mapping *p;

   p = blockheap_alloc(pkg_file_heap);

   p->pkg = str_dup(path);
   p->len = len;
   p->offset = offset;

   if ((p->fd = open(p->pkg, O_RDONLY)) == -1) {
      Log(LOG_ERR, "%s:open:%s %d:%s", __FUNCTION__, p->pkg, errno, strerror(errno));
      pkg_unmap_file(p);
      return NULL;
   }

   if ((p->addr =
        mmap(0, len, PROT_READ | PROT_EXEC, MAP_NOSYNC | MAP_PRIVATE,
             p->fd, offset)) == MAP_FAILED) {
      Log(LOG_ERR, "%s:mmap: %d:%s", __FUNCTION__, errno, strerror(errno));
      pkg_unmap_file(p);
   }

   return p;
}

//
// pkg_import: Scan the contents of a package and add them to the VFS view
//
// XXX: We should add a 'preload' option to jailconf to cache all files in
// XXX: important packages. - jm 12/13/18
int pkg_import(const char *path) {
   char       *tmp = NULL;
   int         infd, outfd;
   int         db_id = -1;
   struct archive *a;
   struct archive_entry *aentry;
   int r;
   int pkgid = -1;
   char _f_type = '-';

   /*
    * Upgrading a package should consist of replace it with
    * one with a higher version number than the old one.
    *
    * A flag will be set on the package, indicating it is to be removed
    * and when the refcnt hits ZERO, it will be unlinked by the system.
    * This won't work until the persistent database is in place
    * and will also require the assistance of the package manager to ensure
    * nothing gets left behind. -- For now, it's not gonna happen
    */
   if (pkg_handle_byname(path)) {
      Log(LOG_CRIT, "BUG: Active package %s was changed, ignoring...");
      return EXIT_SUCCESS;
   }

   if (dconf_get_bool("debug.pkg", 0) == 1)
      Log(LOG_DEBUG, "BEGIN import pkg %s", basename(path));

   // Start transaction
   db_begin();

   // Initialize our libarchive reader
   a = archive_read_new();
   archive_read_support_filter_all(a);
   archive_read_support_format_all(a);
   r = archive_read_open_filename(a, path, 0);

   if (r != ARCHIVE_OK) {
      Log(LOG_ERR, "package %s is not valid: libarchive returned %d", path, r);
      return -1;
   }

   // Add the package to the database & get the pkgid for it
   pkgid = db_pkg_add(path);
   Log(LOG_DEBUG, "package %s appears valid, assigning pkgid %d", path, pkgid);

   // Add the achive's file entries to the database...
   while (TRUE) {
      r = archive_read_next_header(a, &aentry);
      if (r == ARCHIVE_EOF)
         break;

      if (r != ARCHIVE_OK) {
         db_rollback();
         Log(LOG_DEBUG, "pkg_import: libarchive read_next_header error %d: %s", r, archive_error_string(a));
         break;
      }

      const char *_f_name = archive_entry_pathname(aentry);
      const char *_f_owner = archive_entry_uname(aentry);
      const char *_f_group = archive_entry_gname(aentry);
      const uid_t _f_uid = archive_entry_uid(aentry);
      const gid_t _f_gid = archive_entry_gid(aentry);
      const mode_t _f_mode = archive_entry_perm(aentry);
      const char *_f_perm = archive_entry_strmode(aentry);
      const struct stat *st = archive_entry_stat(aentry); 

      // XXX: Determine type more sanely
      if (*_f_perm == 'd')
         _f_type = 'd';
      else if (*_f_perm == 'l')
         _f_type = 'l';
      else
         _f_type = 'f';

      // Add directories...
      if (_f_type == 'd') {
         archive_read_data_skip(a);
         // XXX: Add to database
         continue;
      }

      if (dconf_get_bool("debug.pkg", 0) == 1)
         Log(LOG_DEBUG, "+ %s:%s (user: %d %s) (group: %d %s) mode=%o perms=%s size:%lu@%lu",
             basename(path), _f_name, _f_uid, _f_owner, _f_gid, _f_group, _f_mode, _f_perm, st->st_size, 0);

      if (_f_type == 'l') {
         // XXX: Save symlinks...
      } else {
         db_file_add(pkgid, _f_name, _f_type, _f_uid, _f_gid,
                         _f_owner, _f_group, st->st_size,
                         0, time(NULL), st->st_mode, _f_perm);
      }
   }

   archive_read_close(a);
   if ((r = archive_read_free(a)) != ARCHIVE_OK)
      Log(LOG_ERR, "possible memory leak! archive_read_free() returned %d", r);

   db_commit();

   if (dconf_get_bool("debug.pkg", 0) == 1)
      Log(LOG_INFO, "SUCCESS import pkg %s", basename(path));

   return EXIT_SUCCESS;
}


void pkg_init(void) {
   if (!(pkg_heap = blockheap_create(sizeof(struct pkg_handle),
                         dconf_get_int("tuning.heap.pkg", 128), "pkg"))) {
      Log(LOG_EMERG, "pkg_init(): block allocator failed - pkg");
      raise(SIGTERM);
   }
   if (!(pkg_file_heap = blockheap_create(sizeof(struct pkg_file_mapping),
                         dconf_get_int("tuning.heap.files", 128), "files"))) {
      Log(LOG_EMERG, "pkg_init(): block allocator failed - files");
      raise(SIGTERM);
   }

   // We take care of package file cleanup here too...
   pkg_lifetime = timestr_to_time(dconf_get_str("tuning.timer.pkg_gc", NULL), 60);
   evt_timer_add_periodic(pkg_gc, "gc.pkg", pkg_lifetime);
}

void pkg_fini(void) {
   blockheap_destroy(pkg_heap);
   blockheap_destroy(pkg_file_heap);
}
