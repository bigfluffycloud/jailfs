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
#if	!defined(__MEMORY)
#define	__MEMORY
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "logger.h"

/*
 * memory allocation wrappers
 *	These are lightweight and should catch the common allocation problems
 *	-- One could add pretty easily hooks to a debugging malloc in here
 *	-- or a garbage collector. ;)
 */
static __inline void *mem_alloc(size_t size) {
   register void *ptr;

   if ((ptr = malloc(size)) == NULL) {
//      Log(LOG_ERR, "%s:malloc %d:%s", __FUNCTION__, errno, strerror(errno));
      fprintf(stderr, "%s:malloc %d:%s", __FUNCTION__, errno, strerror(errno));
   }

   memset(ptr, 0, size);

   return ptr;
}

static __inline void *mem_realloc(void *ptr, size_t size) {
   register void *nptr;

   if ((nptr = realloc(ptr, size)) == NULL) {
//      Log(LOG_ERR, "%s:realloc %d:%s", __FUNCTION__, errno, strerror(errno));
      fprintf(stderr, "%s:realloc %d:%s", __FUNCTION__, errno, strerror(errno));
   }
   return nptr;
}

static __inline void mem_free(void *ptr) {
   /*
    * Force a crash if double free or a NULL ptr, should aid debugging 
    */
   assert(ptr != NULL);
   free(ptr);
   ptr = NULL;

   return;
}

static __inline void *mem_calloc(size_t nmemb, size_t size) {
   register void *p;

   assert(nmemb != 0);
   assert(size != 0);

   if ((p = calloc(nmemb, size)) == NULL) {
//      Log(LOG_ERR, "%s:calloc %d:%s", __FUNCTION__, errno, strerror(errno));
      fprintf(stderr, "%s:calloc %d:%s", __FUNCTION__, errno, strerror(errno));
   }
   return p;
}
#endif                                 /* !defined(__MEMORY) */

#if	!defined(__memory_h)
#define	__memory_h
/*
typedef struct alloc_block {
   size_t sz;
   List *lp;
   unsigned long free_blocks;
   void *elems;
} alloc_block;

*/
//extern void *mem_alloc(size_t sz);
//extern void mem_free(void *ptr);
//extern void *mem_realloc(void *ptr, size_t sz);

#endif	// !defined(__memory_h)
