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
   vfs_handle_heap =
       blockheap_create(sizeof(vfs_handle_t),
                        dconf_get_int("tuning.heap.vfs_handle", 128), "vfs_handle");
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

   if ((mountpoint = dconf_get_str("path.mountpoint", NULL)) != NULL)
      umount(mountpoint);

   thread_exit(data);
   return data;
}
