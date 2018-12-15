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
 */
#include <lsd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include "logger.h"
#include "threads.h"
#include "cron.h"
#include "vfs.h"
#include "database.h"
#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
#include <fuse/fuse_opt.h>

// Anywhere VFS recurses, hard limit it to this:
#define	VFS_MAX_RECURSE		16

///////////////////
// Private stuff //
///////////////////
// Heaps & Lists
BlockHeap *vfs_handle_heap = NULL;
BlockHeap *vfs_watch_heap = NULL;
dlink_list vfs_watch_list;
static char *mountpoint = NULL;

// FUSE state
static ev_io vfs_fuse_evt;
static struct fuse_chan *vfs_fuse_chan = NULL;
static struct fuse_session *vfs_fuse_sess = NULL;
static struct fuse_args vfs_fuse_args = { 0, NULL, 0 };

static struct fuse_lowlevel_ops vfs_fuse_ops = {
#if	0
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
   .write = vfs_op_write
#endif	// 0
};

static void vfs_fuse_read_cb(struct ev_loop *loop, ev_io * w, int revents) {
   int         res = 0;
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
}

void vfs_fuse_fini(void) {
   if (vfs_fuse_sess != NULL)
      fuse_session_destroy(vfs_fuse_sess);

   if (vfs_fuse_chan != NULL) {
      fuse_session_remove_chan(vfs_fuse_chan);
      fuse_unmount(dconf_get_str("path.mountpoint", "/"), vfs_fuse_chan);
   }

   if (vfs_fuse_args.allocated)
      fuse_opt_free_args(&vfs_fuse_args);
}

void vfs_fuse_init(void) {
   Log(LOG_DEBUG, "mountpoint: %s/%s", get_current_dir_name(), conf.mountpoint);

   if ((vfs_fuse_chan = fuse_mount(conf.mountpoint, &vfs_fuse_args)) == NULL) {
      Log(LOG_EMERG, "FUSE: mount error");
      conf.dying = 1;
      raise(SIGTERM);
   }

   if ((vfs_fuse_sess = fuse_lowlevel_new(&vfs_fuse_args, &vfs_fuse_ops,
                                          sizeof(vfs_fuse_ops), NULL)) != NULL) {
      fuse_session_add_chan(vfs_fuse_sess, vfs_fuse_chan);
   } else {
      Log(LOG_EMERG, "FUSE: unable to create session");
      conf.dying = 1;
      raise(SIGTERM);
   }

   /*
    * Register an interest in events on the fuse fd 
    */
   ev_io_init(&vfs_fuse_evt, vfs_fuse_read_cb, fuse_chan_fd(vfs_fuse_chan), EV_READ);
   ev_io_start(evt_loop, &vfs_fuse_evt);

   /*
    * Set up our various blockheaps 
    */
   vfs_handle_heap = blockheap_create(sizeof(vfs_handle_t), dconf_get_int("tuning.heap.vfs_handle", 128), "vfs_handle");
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

   if ((i = db_query(QUERY_INODE, "SELECT * FROM files WHERE inode = %d", (u_int32_t) ino)) != NULL) {
      sb.st_ino = ino;
      sb.st_mode = i->st_mode;
      sb.st_size = i->st_size;
      sb.st_uid = i->st_uid;
      sb.st_gid = i->st_gid;
      blockheap_free(vfs_inode_heap, i);
   }

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
   fh = blockheap_alloc(vfs_handle_heap);
   fi->fh = ((uint64_t) fh);
   pkg_inode_t *i;
   struct stat sb;
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);

   // Check if a spillover file exists
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

   blockheap_free(vfs_inode_heap, i);
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
      /*
       * XXX: stat 
       */
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
      /*
       * Skip over hidden/disabled items 
       */
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

   
/////////////////
// Public crap //
/////////////////

// garbage collector
void vfs_garbagecollect(void) {
   blockheap_garbagecollect(vfs_handle_heap);
   blockheap_garbagecollect(vfs_watch_heap);
}

////////////
// thread //
////////////
// thread:creator
void *thread_vfs_init(void *data) {
    dict *args = (dict *)data;
    thread_entry(data);

    // Schedule garbage collection
    evt_timer_add_periodic(vfs_garbagecollect, "gc:vfs", dconf_get_int("tuning.timer.vfs_gc", 1200));
    //hook_register("gc", vfs_garbagecollect);

    // Mount the virtual file system
    if ((mountpoint = dconf_get_str("path.mountpoint", NULL)) == NULL) {
       Log(LOG_EMERG, "mountpoint not configured. exiting!");
       raise(SIGTERM);
    }

    mimetype_init();
    umount(conf.mountpoint);

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
    vfs_fuse_init();

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

   vfs_watch_fini();
   vfs_fuse_init();
   vfs_inode_fini();

   if ((mountpoint = dconf_get_str("path.mountpoint", NULL)) != NULL)
      umount(mountpoint);

   thread_exit(data);
   return data;
}
