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
 * src/gc.c
 * 	Garbage collection tasks managed by the main thread
 */
#include <lsd.h>
#include "vfs.h"
#include "shell.h"

extern BlockHeap *dlink_node_heap;

int gc_all(void) {
     int freed = 0;
     freed += api_gc();
     freed += shell_gc();
     freed += vfs_gc();

     blockheap_garbagecollect(dlink_node_heap);
     return 0;
}
