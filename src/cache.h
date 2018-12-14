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
#if	!defined(__CACHE_H)
#define	__CACHE_H
#include <lsd/dict.h>

struct CacheItem {
   int dirty;
   dict *cache;
};
typedef struct CacheItem CacheItem_t;

extern void *thread_cache_init(void *data);
extern void *thread_cache_fini(void *data);

#endif	// !defined(__CACHE_H)
