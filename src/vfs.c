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
 *
 * src/vfs.c:
 *	Virtual File System services.
 * Here we synthesize a file system view for the container.
 *
 *    author: joseph@bigfluffy.cloud
 * copyright: 2008-2018 Bigfluffy.cloud, All rights reserved.
 *   license: MIT
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <lsd/lsd.h>
#include <sys/signal.h>
#include <errno.h>
#include <dirent.h>
#include "conf.h"
#include "shell.h"
#include "threads.h"
#include "cron.h"
#include "vfs.h"
#include "database.h"
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
#include <fuse/fuse_opt.h>

// Anywhere VFS recurses, hard limit it to this:
#define	VFS_MAX_RECURSE		16

///////////////////
// Private stuff //
///////////////////
static BlockHeap *heap_vfs_cache = NULL,
          *heap_vfs_handle = NULL,
          *heap_vfs_inode = NULL,
          *heap_vfs_watch = NULL;
dlink_list vfs_watch_list;
static pthread_mutex_t cache_mutex;
static char *mountpoint = NULL;
static char *cache_path = NULL;
static dict *cache_dict = NULL;

// FUSE state
static ev_io vfs_fuse_evt;
static struct fuse_chan *vfs_fuse_chan = NULL;
static struct fuse_session *vfs_fuse_sess = NULL;
static struct fuse_args vfs_fuse_args = { 0, NULL, 0 };

////////////
// inodes //
////////////
#if	0
static void fill_statbuf(ext2_ino_t ino, pkg_inode_t * inode, struct stat *st) {
   /*
    * st_dev 
    */
   st->st_ino = ino;
   st->st_mode = inode->i_mode;
   st->st_nlink = inode->i_links_count;
   st->st_uid = inode->i_uid;          /* add in uid_high */
   st->st_gid = inode->i_gid;          /* add in gid_high */
   /*
    * st_rdev 
    */
   st->st_size = inode->i_size;
   st->st_blksize = EXT2_BLOCK_SIZE(fs->super);
   st->st_blocks = inode->i_blocks;

   /*
    * We don't have to implement nanosecs, fs's which don't can return
    * * 0 here
    */
   /*
    * Using _POSIX_C_SOURCE might also work 
    */
#ifdef __APPLE__
   st->st_atimespec.tv_sec = inode->i_atime;
   st->st_mtimespec.tv_sec = inode->i_mtime;
   st->st_ctimespec.tv_sec = inode->i_ctime;
   st->st_atimespec.tv_nsec = 0;
   st->st_mtimespec.tv_nsec = 0;
   st->st_ctimespec.tv_nsec = 0;
#else
   st->st_atime = inode->i_atime;
   st->st_mtime = inode->i_mtime;
   st->st_ctime = inode->i_ctime;
#ifdef __FreeBSD__
   st->st_atimespec.tv_nsec = 0;
   st->st_mtimespec.tv_nsec = 0;
   st->st_ctimespec.tv_nsec = 0;
#else
   st->st_atim.tv_nsec = 0;
   st->st_mtim.tv_nsec = 0;
   st->st_ctim.tv_nsec = 0;
#endif
#endif
}
#endif


////////////////////
// fuse interface //
////////////////////
static void vfs_fuse_read_cb(struct ev_loop *loop, ev_io * w, int revents) {
   int         res = 0;
#if	0
   struct fuse_chan *ch = fuse_session_next_chan(vfs_fuse_sess, NULL);
   struct fuse_chan *tmpch = ch;
   size_t      bufsize = fuse_chan_bufsize(ch);
   char       *buf;

   if (!(buf = mem_alloc(bufsize))) {
      Log(LOG_EMERG, "fuse: failed to allocate read buffer\n");
      conf.dying = 1;
      return;
   }

   res = fuse_chan_recv(&tmpch, buf, bufsize);

   if (!(res == -EINTR || res <= 0))
      fuse_session_process(vfs_fuse_sess, buf, res, tmpch);

   mem_free(buf);
   fuse_session_reset(vfs_fuse_sess);
#endif
}

/* Write-type operations which should return EROFS */
void vfs_op_setattr(fuse_req_t req, fuse_ino_t ino,
                             struct stat *attr, int to_set, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_mknod(fuse_req_t req, fuse_ino_t ino,
                           const char *name, mode_t mode, dev_t rdev) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_mkdir(fuse_req_t req, fuse_ino_t ino, const char *name, mode_t mode) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_symlink(fuse_req_t req, const char *link, fuse_ino_t parent, const char *name) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_unlink(fuse_req_t req, fuse_ino_t ino, const char *name) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_rmdir(fuse_req_t req, fuse_ino_t ino, const char *namee) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                            fuse_ino_t newparent, const char *newname) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char *newname) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                           size_t size, off_t off, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                              const char *value, size_t size, int flags) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_op_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name) {
   /*
    * XXX: which is proper: ENOSUP, EACCES, ENOATTR? 
    */
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_op_create(fuse_req_t req, fuse_ino_t ino, const char *name,
                            mode_t mode, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_op_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   pkg_inode_t *i;
   struct stat sb;
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);

/*
   if ((i = db_query(QUERY_INODE, "SELECT * FROM files WHERE inode = %d", (u_int32_t) ino)) != NULL) {
      sb.st_ino = ino;
      sb.st_mode = i->st_mode;
      sb.st_size = i->st_size;
      sb.st_uid = i->st_uid;
      sb.st_gid = i->st_gid;
      blockheap_free(heap_vfs_inode, i);
   }
*/
   // Did we get a valid response?
   if (sb.st_ino) {
      fuse_reply_attr(req, &sb, 0.0);
      Log(LOG_DEBUG, "got attr.st_ino: %d <mode:%o> <size:%lu> (%d:%d)", sb.st_ino,
          sb.st_mode, sb.st_size, sb.st_uid, sb.st_gid);
   } else {
      Log(LOG_DEBUG, "couldn't find attr.st_ino");
      fuse_reply_err(req, ENOENT);
   }
}

void vfs_op_access(fuse_req_t req, fuse_ino_t ino, int mask) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, 0);             /* success */
}

void vfs_op_readlink(fuse_req_t req, fuse_ino_t ino) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOSYS);
}

void vfs_op_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOENT);
}

void vfs_op_readdir(fuse_req_t req, fuse_ino_t ino,
                             size_t size, off_t off, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOENT);
}

void vfs_op_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOENT);
}

void vfs_op_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   struct vfs_handle *fh;
   fh = blockheap_alloc(heap_vfs_handle);
   fi->fh = ((uint64_t) fh);
   pkg_inode_t *i;
   struct stat sb;
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);

   // Check if a spillover file exists
i =  0;
/*
   if ((i = db_query(QUERY_INODE, "SELECT * FROM spillover WHERE inode = %d", (u_int32_t) ino))) {
     // if file doesn't exist in cache but exist in spillover, extract to cache
     //
     // if file exists in cache; use it
     //
   // If not, maybe this exists in a package?
   } else if ((i = db_query(QUERY_INODE, "SELECT * FROM files WHERE inode = %d", (u_int32_t) ino))) {
     // if file doesn't exist in cache but exists in pkg, extract it to cache
     //
     // if file exists in cache; use it
     //
   }
*/
   // Nope, it doesn't exist
   if (!i) {
      // If process didn't use O_CREAT, return not found
      if (!(fi->flags & O_CREAT)) {
         fuse_reply_err(req, ENOENT);
      } else {
         // XXX: We should create a new fill in spillover
         // and return it's inode. For now we return read-only
         fuse_reply_err(req, EROFS);
      }
   }

   blockheap_free(heap_vfs_inode, i);
   fuse_reply_open(req, fi);
}

void vfs_op_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, 0);             /* success */
}

void vfs_op_read(fuse_req_t req, fuse_ino_t ino,
                          size_t size, off_t off, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
/*      reply_buf_limited(req, hello_str, strlen(hello_str), off, size); */
   fuse_reply_err(req, EBADF);
}

void vfs_op_statfs(fuse_req_t req, fuse_ino_t ino) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOSYS);
}

void vfs_op_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_op_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_op_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
   struct fuse_entry_param e;

   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
#if	0
   if (parent != 1)                    /* XXX: need checks here */
#endif
      fuse_reply_err(req, ENOENT);
#if	0
   else {
      memset(&e, 0, sizeof(e));
      e.ino = 2;                       /* Inode number */
      e.attr_timeout = 1.0;
      e.entry_timeout = 1.0;
      // XXX: stat 
      vfs_op_stat(e.ino, &e.attr);
      fuse_reply_entry(req, &e);
   }
#endif
}

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
      // Skip over hidden/disabled items 
      if (r->d_name[0] == '.')
         continue;
      memset(buf, 0, PATH_MAX);
      snprintf(buf, PATH_MAX - 1, "%s/%s", path, r->d_name);

      if (is_dir(buf)) {
         if (++depth < VFS_MAX_RECURSE)
            vfs_dir_walk_recurse(buf, depth);
         else
            Log(LOG_INFO, "%s: reached maximum depth (%d) in %s", __FUNCTION__, VFS_MAX_RECURSE, path);
      } else {
         ext = strrchr(buf, '.');
         

         if (ext == NULL)
            return;

         pkg_import(buf);
      }
   }

   closedir(d);
}

int vfs_dir_walk(void) {
   char        buf[PATH_MAX];
   char       *p;

   memcpy(buf, dconf_get_str("path.pkg", "/pkg"), PATH_MAX);

   // recurse each part of the optionally ':' seperated list 
   for (p = strtok(buf, ":\n"); p; p = strtok(NULL, ":\n")) {
      // Run the recursive walker 
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

static struct fuse_lowlevel_ops vfs_fuse_ops = {
   .lookup = vfs_op_lookup,
   .readlink = vfs_op_readlink,
   .open = vfs_op_open,
   .release = vfs_op_release,
   .read = vfs_op_read,
   .getattr = vfs_op_getattr,
   .access = vfs_op_access,
   .statfs = vfs_op_statfs,
//   .getxattr = vfs_op_getxattr,
//   .setxattr = vfs_op_setxattr,
//   .listxattr = vfs_op_listxattr,
//   .removexattr = vfs_op_removexatr,
   .opendir = vfs_op_opendir,
   .readdir = vfs_op_readdir,
   .releasedir = vfs_op_releasedir,
   .create = vfs_op_create,
   .mknod = vfs_op_mknod,
   .mkdir = vfs_op_mkdir,
   .symlink = vfs_op_symlink,
   .unlink = vfs_op_unlink,
   .rmdir = vfs_op_rmdir,
   .rename = vfs_op_rename,
   .link = vfs_op_link,
};
   
/////////////////
// Public crap //
/////////////////

void vfs_fuse_fini(void) {
   if (vfs_fuse_sess != NULL)
      fuse_session_destroy(vfs_fuse_sess);

   if (vfs_fuse_chan != NULL) {
#if	0
      fuse_session_remove_chan(vfs_fuse_chan);
#endif
      fuse_unmount(vfs_fuse_chan);
   }

   if (vfs_fuse_args.allocated)
      fuse_opt_free_args(&vfs_fuse_args);
}

void vfs_fuse_init(void) {
   Log(LOG_DEBUG, "mountpoint: (%s)%s", get_current_dir_name(), mountpoint);

   if ((vfs_fuse_chan = fuse_mount(mountpoint, &vfs_fuse_args)) == NULL) {
      Log(LOG_EMERG, "FUSE: mount error");
      conf.dying = 1;
      raise(SIGTERM);
   }
#if	0
   if ((vfs_fuse_sess = fuse_lowlevel_new(&vfs_fuse_args, &vfs_fuse_ops,
                                          sizeof(vfs_fuse_ops), NULL)) != NULL) {
      fuse_session_add_chan(vfs_fuse_sess, vfs_fuse_chan);
   } else {
      Log(LOG_EMERG, "FUSE: unable to create session");
      conf.dying = 1;
      raise(SIGTERM);
   }
   // Register an interest in events on the fuse fd 
   ev_io_init(&vfs_fuse_evt, vfs_fuse_read_cb, fuse_chan_fd(vfs_fuse_chan), EV_READ);
   ev_io_start(evt_loop, &vfs_fuse_evt);
#endif
}

// garbage collector
int vfs_gc(void) {
   blockheap_garbagecollect(heap_vfs_cache);
   blockheap_garbagecollect(heap_vfs_handle);
   blockheap_garbagecollect(heap_vfs_inode);
   blockheap_garbagecollect(heap_vfs_watch);
   return 0;
}

////////////
// thread //
////////////
// thread:creator
void *thread_vfs_init(void *data) {
    dict *args = (dict *)data;
    thread_entry(data);
    char *cache = NULL;

    thread_entry((dict *)data);
    cache = dconf_get_str("path.cache", NULL);
    cache_dict = dict_new();
    if (!(heap_vfs_cache = blockheap_create(sizeof(vfs_cache_entry), dconf_get_int("tuning.heap.files", 1024), "cache entries"))) {
       Log(LOG_EMERG, "vfs_init: block allocator failed");
       raise(SIGABRT);
    }

    if (!(heap_vfs_handle = blockheap_create(sizeof(vfs_handle_t), dconf_get_int("tuning.heap.vfs_handle", 128), "vfs_handle"))) {
       Log(LOG_EMERG, "vfs_init: block allocator failed");
       raise(SIGABRT);
    }

    if (!(heap_vfs_watch = blockheap_create(sizeof(vfs_watch_t), dconf_get_int("tuning.heap.vfs_watch", 32), "vfs_watch"))) {
       Log(LOG_EMERG, "vfs_init: block allocator failed");
       raise(SIGABRT);
    }

    if (!(heap_vfs_inode = blockheap_create(sizeof(struct pkg_inode), dconf_get_int("tuning.heap.inode", 128), "pkg"))) {
       Log(LOG_EMERG, "vfs_init(): block allocator failed");
       raise(SIGABRT);
    }

    // If .keepme exists in cachedir (from git), remove it or mount will fail
    char tmppath[PATH_MAX];
    memset(tmppath, 0, sizeof(tmppath));
    snprintf(tmppath, sizeof(tmppath) - 1, "%s/.keepme", cache);
    if (file_exists(tmppath))
       unlink(tmppath);

    // Are we configured to use a tmpfs or host fs for cache?
    if (strcasecmp("tmpfs", dconf_get_str("cache.type", NULL)) == 0) {
       if (cache != NULL) {
          int rv = -1;

          // Attempt mounting tmpfs
          if ((rv = mount("jailfs-cache", cache, "tmpfs", 0, NULL)) != 0) {
             Log(LOG_ERR, "mounting tmpfs on cache-dir %s failed: %d (%s)", cache, errno, strerror(errno));
             exit(1);
          }
       }
    }

    // Schedule garbage collection
    evt_timer_add_periodic(vfs_gc, "gc:vfs", dconf_get_int("tuning.timer.vfs_gc", 1200));
    //hook_register_interest("gc", vfs_gc);

    // Mount the virtual file system
    if ((mountpoint = dconf_get_str("path.mountpoint", NULL)) == NULL) {
       Log(LOG_EMERG, "mountpoint not configured. exiting!");
       raise(SIGTERM);
    }

    mimetype_init();
    umount(mountpoint);

    // XXX: Initialize FUSE stuff
    struct fuse_args margs = FUSE_ARGS_INIT(0, NULL);
    vfs_fuse_args = margs;

    if ((fuse_opt_add_arg(&vfs_fuse_args, dconf_get_str("jail.name", NULL)) == -1 ||
         fuse_opt_add_arg(&vfs_fuse_args, "-o") == -1 ||
         fuse_opt_add_arg(&vfs_fuse_args, "nonempty,allow_other") == -1)) {
       Log(LOG_EMERG, "Failed to set FUSE options.");
       raise(SIGTERM);
    }

    umount(mountpoint);
//    vfs_fuse_init();

    // Add inotify watchers for paths in %{path.pkg}
    if (dconf_get_bool("pkgdir.inotify", 0) == 1)
       vfs_watch_init();

    // Load all packages in %{path.pkg}} if enabled
    if (dconf_get_bool("pkgdir.prescan", 0) == 1)
       vfs_dir_walk();

    // Main loop for thread
    while (!conf.dying) {
       sleep(3);
       pthread_yield();
    }

    return data;
}

// thread:destructor
void *thread_vfs_fini(void *data) {
   dict *args = (dict *)data;
   char *mp = NULL;

   if ((mountpoint = dconf_get_str("path.mountpoint", NULL)) != NULL)
      umount(mountpoint);

   if (strcasecmp("tmpfs", dconf_get_str("cache.type", NULL)) == 0) {
      if ((mp = dconf_get_str("path.cache", NULL)) != NULL)
         umount(mp);
   }
   vfs_fuse_fini();
   dict_free(cache_dict);
   blockheap_destroy(heap_vfs_cache);
   blockheap_destroy(heap_vfs_inode);
   blockheap_destroy(heap_vfs_watch);
   thread_exit((dict *)data);
   return data;
}

/////////////////////
// inotify support //
/////////////////////
#define	INOTIFY_BUFSIZE	((sizeof(struct inotify_event) + FILENAME_MAX) * 1024)
static int  vfs_inotify_fd = 0;
static ev_io vfs_inotify_evt;

static dlink_node *vfs_watch_findnode(vfs_watch_t * watch) {
   dlink_node *ptr, *tptr;

   DLINK_FOREACH_SAFE(ptr, tptr, vfs_watch_list.head) {
      if ((vfs_watch_t *) ptr->data == watch)
         return ptr;
   }

   return NULL;
}

static dlink_node *vfs_watch_findnode_byfd(const int fd) {
   dlink_node *ptr, *tptr;

   DLINK_FOREACH_SAFE(ptr, tptr, vfs_watch_list.head) {
      if (((vfs_watch_t *) ptr->data)->fd == fd)
         return ptr;
   }

   return NULL;
}

/*
 *    function: vfs_inotify_evt_get
 * description: process the events inotify has given us
 */
void vfs_inotify_evt_get(struct ev_loop *loop, ev_io * w, int revents) {
   ssize_t     len, i = 0;
   char        buf[INOTIFY_BUFSIZE] = { 0 };
   char        path[PATH_MAX];
   dlink_node *ptr;
   char       *ext;

   len = read(w->fd, buf, INOTIFY_BUFSIZE);

   while (i < len) {
      struct inotify_event *e = (struct inotify_event *)&buf[i];

      if (e->name[0] == '.')
         continue;

      if ((ptr = vfs_watch_findnode_byfd(e->wd)) == NULL) {
         Log(LOG_ERR, "got watch for unregistered fd %d.. wtf?", e->wd);
         return;
      }

      /*
       * If we don't have a name, this is useless event... 
       */
      if (!e->len)
         return;

      memset(path, 0, PATH_MAX);
      snprintf(path, PATH_MAX - 1, "%s/%s", ((vfs_watch_t *) ptr->data)->path, e->name);

      if (e->mask & IN_CLOSE_WRITE || e->mask & IN_MOVED_TO)
         pkg_import(path);
      else if (e->mask & IN_DELETE || e->mask & IN_MOVED_FROM)
         pkg_forget(path);
      else if (e->mask & IN_DELETE_SELF) {
         vfs_watch_remove((vfs_watch_t *) ptr->data);
      }

      i += sizeof(struct inotify_event) + e->len;
   }
}
vfs_watch_t *vfs_watch_add(const char *path) {
   vfs_watch_t *wh = blockheap_alloc(heap_vfs_watch);

   wh->mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE | IN_DELETE_SELF;
   memcpy(wh->path, path, sizeof(wh->path));

   if ((wh->fd = inotify_add_watch(vfs_inotify_fd, path, wh->mask)) <= 0) {
      Log(LOG_ERR, "failed creating vfs watcher for path %s", wh->path);
      blockheap_free(heap_vfs_watch, wh);
      return NULL;
   }

   dlink_add_tail_alloc(wh, &vfs_watch_list);
   return wh;
}

/* XXX: We need to scan the watch lists and remove subdirs too -bk */

int vfs_watch_remove(vfs_watch_t * watch) {
   int         rv;
   dlink_node *ptr;

   if ((rv = inotify_rm_watch(watch->fd, vfs_inotify_fd)) != 0) {
      Log(LOG_ERR, "error removing watch for %s (fd: %d)", watch->path, watch->fd);
   }

   if ((ptr = vfs_watch_findnode(watch)) != NULL) {
      dlink_delete(ptr, &vfs_watch_list);
      blockheap_free(heap_vfs_watch, ptr->data);
   }

   return rv;
}

int vfs_watch_init(void) {
   char        buf[PATH_MAX];
   char       *p;
   if (vfs_inotify_fd)
      return -1;

   /*
    * Try to initialize inotify interface 
    */
   if ((vfs_inotify_fd = inotify_init()) == -1) {
      Log(LOG_ERR, "%s:inotify_init %d:%s", __FUNCTION__, errno, strerror(errno));
      return -2;
   }

   /*
    * Setup the socket callback 
    */
   ev_io_init(&vfs_inotify_evt, vfs_inotify_evt_get, vfs_inotify_fd, EV_READ);
   ev_io_start(evt_loop, &vfs_inotify_evt);


   /*
    * add all the paths in the config file 
    */
   memcpy(buf, dconf_get_str("path.pkg", "/pkg"), PATH_MAX);
   if (strchr(buf, ':') == NULL) {
      for (p = strtok(buf, ":\n"); p; p = strtok(NULL, ":\n")) {
         vfs_watch_add(p);
         Log(LOG_INFO, "inotify watcher started for %s", p);
       }
   } else {
      vfs_watch_add(buf);
      Log(LOG_INFO, "inotify watcher started for %s", buf);
   }

   return 0;
}

////////////////
// cache bits //
////////////////

// backend function that does the actual heavy lifting...
int vfs_add_path(const char type, int pkgid, const char *path, uid_t uid, gid_t gid, const char *owner, const char *group,
                 mode_t mode, size_t size, time_t ctime) {
    vfs_cache_entry *fe = NULL;

    if (pkgid < 1) {
       Log(LOG_ERR, "vfs_add_path: pkgid %d is not valid (<1)", pkgid);
       return -1;
    }

    if (path == NULL || *path == NULL) {
       Log(LOG_ERR, "vfs_add_path: in pkg %d got NULL path", pkgid);
       return -1;
    }


    if ((type != 'd') && (fe = vfs_find(path))) {
       Log(LOG_ERR, "vfs_add_path: %d:%s already exists in pkg %d", pkgid, path, fe->pkgid);
       return -1;
    }

    if (!(fe = blockheap_alloc(heap_vfs_cache))) {
       Log(LOG_ERR, "vfs_add_path: error allocating memory");
       return -1; 
    }

    // Set up the cache entry structure
    memset(fe, 0, sizeof(vfs_cache_entry));
    memcpy(fe->path, path, sizeof(fe->path)-1);
    memcpy(fe->owner, owner, sizeof(fe->owner)-1);
    memcpy(fe->group, group, sizeof(fe->group)-1);
    fe->pkgid = pkgid;
    fe->uid = uid;
    fe->gid = gid;
    fe->mode = mode;
    fe->size = size;
    fe->ctime = ctime;

    switch (type) {
       case 'd':
          fe->type = PKG_FTYPE_DIR;
          break;
       case 'f':
          fe->type = PKG_FTYPE_FILE;
          break;
       case 'l':
          fe->type = PKG_FTYPE_LINK;
          break;
       case 'D':
          fe->type = PKG_FTYPE_DEV;
          break;
       case 'F':
          fe->type = PKG_FTYPE_FIFO;
          break;
    }

    Log(LOG_DEBUG, "vfs_add_path: Added <%d> %c:%s", pkgid, type, path);
    return 0;
}

// Find a cache entry
vfs_cache_entry *vfs_find(const char *path) {
    return NULL;
}

int vfs_unpack_tempfile(vfs_cache_entry *fe) {
    char *path = NULL;

    if (fe == NULL)
       return -1;

    if (fe->refcnt > 0) {
       // Find handle for existing file and return it instead
    }

    // We haven't extracted it yet...
    if (fe->cache_path[0] == NULL) {
       // Extract it
       path = pkg_extract_file(fe->pkgid, fe->path);
       memcpy(fe->cache_path, path, sizeof(fe->cache_path)-1);
       // register the unpacked file
    }

    fe->refcnt++;
    return 0;
}
