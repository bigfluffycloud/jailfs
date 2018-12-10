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
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdarg.h>
#include <pthread.h>
#include "balloc.h"
#include "conf.h"
#include "logger.h"
#include "memory.h"
#include "pkg.h"
#include "pkg_db.h"
#include "util.h"
#define	__VFS_C
#include "vfs.h"

u_int32_t   vfs_root_inode;

// Block allocator heaps
BlockHeap  *vfs_handle_heap = NULL;
BlockHeap  *vfs_watch_heap = NULL;

// Doubly-linked list
dlink_list  vfs_watch_list;

/* We do not want to infinitely recurse... */
#define	MAX_RECURSE	16

static void vfs_dir_walk_recurse(const char *path, int depth) {
   DIR        *d;
   struct dirent *r;
   char       *ext;
   char        buf[PATH_MAX];

   if ((d = opendir(path)) == NULL) {
      Log(LOG_ERR, "opendir %s failed, skipping", path);
      return;
   }

   while ((r = readdir(d)) != NULL) {
      /*
       * Skip over hidden/disabled items 
       */
      if (r->d_name[0] == '.')
         continue;
      memset(buf, 0, PATH_MAX);
      snprintf(buf, PATH_MAX - 1, "%s/%s", path, r->d_name);

      if (is_dir(buf)) {
         if (++depth < MAX_RECURSE)
            vfs_dir_walk_recurse(buf, depth);
         else
            Log(LOG_INFO, "%s: reached maximum depth (%d) in %s", __FUNCTION__, MAX_RECURSE, path);
      } else {
         ext = strrchr(buf, '.');
         

         if (ext == NULL)
            return;

         pkg_import(buf);
      }
   }

   closedir(d);
}
#undef MAX_RECURSE

int vfs_dir_walk(void) {
   char        buf[PATH_MAX];
   char       *p;

   memcpy(buf, dconf_get_str("path.pkg", "/pkg"), PATH_MAX);

   /*
    * recurse each part of the optionally ':' seperated list 
    */
   for (p = strtok(buf, ":\n"); p; p = strtok(NULL, ":\n")) {
      /*
       * Run the recursive walker 
       */
      vfs_dir_walk_recurse(p, 1);
   }
   return EXIT_SUCCESS;
}

/* Be sure to free these :O */
struct vfs_lookup_reply {
   u_int32_t	seq;
   u_int32_t	status;
   int		pkgid;		// Package ID if appropriate
   int		fileid;		// File ID from database
};
typedef struct vfs_lookup_reply vfs_lookup_reply;

vfs_lookup_reply *vfs_resolve_path(const char *path) {
    vfs_lookup_reply *res = mem_alloc(sizeof(vfs_lookup_reply));

    // XXX: Locate the file and return vfs_file handle for it
    // - Look in jail/config/
    // - Look in jail/state/*.spill
    // - Look in jail/pkg/
    // - Look in ../pkg/
    // If not found, create in spillover (if O_CREAT) or return ENOENT.
    if (dconf_get_bool("debug.vfs", 0) == 1) {
       Log(LOG_DEBUG, "vfs:resolve_path(%s): seq<%d> status<%d> pkg<%d> file<%d>",
          res->seq, res->status, res->pkgid, res->fileid);
    }
    return res;
}


void *thread_vfs_init(void *data) {
   char *mp = NULL;

   vfs_inode_init();
   // Figure out where we're supposed to build this jail
   if (!conf.mountpoint)
      conf.mountpoint = dconf_get_str("path.mountpoint", "chroot/");

   // XXX: TODO: We need to unlink mountpoint/.keepme if it exist before calling fuse...
   struct fuse_args margs = FUSE_ARGS_INIT(0, NULL);
   vfs_fuse_args = margs;
   // The fuse_mount() options get modified, so we always rebuild it 
   if ((fuse_opt_add_arg(&vfs_fuse_args, dconf_get_str("jailname", NULL)) == -1 ||
        fuse_opt_add_arg(&vfs_fuse_args, "-o") == -1 ||
        fuse_opt_add_arg(&vfs_fuse_args, "nonempty,allow_other") == -1))
      Log(LOG_ERR, "Failed to set FUSE options.");

   // Insure nothing is mounted on our mountpoint
   // XXX: Someday we will support overlay mode which will work like so:
   //	Check underlying fs, if file exists, serve it.
   //	Check spillover - Send it.
   //	Check packaes - Send it.
   umount(conf.mountpoint);
   vfs_fuse_init();

   // mime file type services
   mimetype_init();

   // Add inotify watchers for paths in %{path.pkg}
   if (dconf_get_bool("pkgdir.inotify", 0) == 1)
      vfs_watch_init();

   // Load all packages in %{path.pkg}} if enabled
   if (dconf_get_bool("pkgdir.prescan", 0) == 1)
      vfs_dir_walk();
   
   while (!conf.dying) {
      pthread_yield();
      sleep(3);
   }
   return NULL;
}

void *thread_vfs_fini(void *data) {
   char *mp = NULL;
   vfs_watch_fini();
   vfs_fuse_fini();
   vfs_inode_fini();

   // Unmount the mountpoint
   if ((mp = dconf_get_str("path.mountpoint", NULL)) != NULL)
      umount(mp);

   return NULL;
}
