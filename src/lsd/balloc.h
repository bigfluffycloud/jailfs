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
 * Copyright (C) 2005 William Pitcock, et al.
 * See balloc.c for original license
 * Data structures for the block allocator.
 * $Id: balloc.h 7779 2007-03-03 13:55:42Z pippijn $
 */
#ifndef __BALLOC_H
#define __BALLOC_H
#include <sys/types.h>
#include <stdlib.h>
#include "dlink.h"
struct Block {
   size_t      alloc_size;
   struct Block *next;                 /* Next in our chain of blocks */
   void       *elems;                  /* Points to allocated memory */
   dlink_list  free_list;
   dlink_list  used_list;
};
typedef struct Block Block;

struct MemBlock {
#ifdef DEBUG_BALLOC
   unsigned long magic;
#endif
   dlink_node  self;
   Block      *block;                  /* Which block we belong to */
};

typedef struct MemBlock MemBlock;

/* information for the root node of the heap */
struct BlockHeap {
   dlink_node  hlist;
   char        name[64];               /* heap name */
   size_t      elemSize;               /* Size of each element to be stored */
   unsigned long elemsPerBlock;        /* Number of elements per block */
   unsigned long blocksAllocated;      /* Number of blocks allocated */
   unsigned long freeElems;            /* Number of free elements */
   Block      *base;                   /* Pointer to first block */
};
typedef struct BlockHeap BlockHeap;

// Allocate/free a block (ptr) from the BlocKHeap (bh)
extern void *blockheap_alloc(BlockHeap *bh);
extern int  blockheap_free(BlockHeap *bh, void *ptr);

// Create/destroy BlockHeap objects
extern BlockHeap *blockheap_create(size_t elemsize, int elemsperblock, const char *name);
extern int  blockheap_destroy(BlockHeap *bh);

// Garbage collection
extern int blockheap_garbagecollect(BlockHeap *bh);

// Accessory functions
extern void blockheap_init(void);
extern void blockheap_usage(BlockHeap *bh, size_t *bused, size_t *bfree, size_t *bmemusage);

#endif	// !defined(__BALLOC_H)
