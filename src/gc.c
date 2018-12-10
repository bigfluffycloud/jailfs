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
#include "memory.h"
#include "balloc.h"
#include "logger.h"
#include "str.h"
#include "cache.h"
#include "dlink.h"
#include "pkg.h"
#include "shell.h"
#include "vfs.h"

extern BlockHeap *dlink_node_heap;
extern BlockHeap *pkg_heap;
extern BlockHeap *pkg_file_heap;
extern BlockHeap *cache_entry_heap;
extern BlockHeap *shell_hints_heap;

int gc_all(void) {
     int freed = 0;
     // XXX: Add some code to balloc:bh_garbagecollect to return freed items
     blockheap_garbagecollect(cache_entry_heap);
     blockheap_garbagecollect(dlink_node_heap);
     blockheap_garbagecollect(pkg_heap);
     blockheap_garbagecollect(pkg_file_heap);
     blockheap_garbagecollect(shell_hints_heap);
     blockheap_garbagecollect(vfs_handle_heap);
     blockheap_garbagecollect(vfs_watch_heap);
     blockheap_garbagecollect(vfs_inode_heap);
     return 0;
}
