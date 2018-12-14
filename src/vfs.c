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
#include "vfs.h"

// Anywhere VFS recurses, hard limit it to this:
#define	VFS_MAX_RECURSE		16

BlockHeap *vfs_handle_heap = NULL;
BlockHeap *vfs_watch_heap = NULL;
dlink_list vfs_watch_list;

////////////
// thread //
////////////
// thread:creator
void *thread_vfs_init(void *data) {
    dict *args = (dict *)data;
    thread_entry(data);

    while (!conf.dying) {
       sleep(3);
       pthread_yield();
    }

    return data;
}

// thread:destructor
void *thread_vfs_fini(void *data) {
   dict *args = (dict *)data;

   thread_exit(data);
   return data;
}
