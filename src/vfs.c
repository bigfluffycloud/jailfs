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
BlockHeap *vfs_handle_heap = NULL;
BlockHeap *vfs_watch_heap = NULL;
dlink_list vfs_watch_list;
static char *mountpoint = NULL;
static struct fuse_args vfs_fuse_args = { 0, NULL, 0 };

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
