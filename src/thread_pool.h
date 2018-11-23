/*
 * tk/servers/fs-pkg:
 *	Package filesystem. Allows mounting various package files as
 *   a read-only (optionally with spillover file) overlay filesystem
 *   via FUSE or (eventually) LD_PRELOAD library.
 *
 * Copyright (C) 2012-2018 BigFluffy.Cloud <joseph@bigfluffy.cloud>
 *
 * Distributed under a BSD license. Send bugs/patches by email or
 * on github - https://github.com/bigfluffycloud/fs-pkg/
 *
 * No warranty of any kind. Good luck!
 */
#include <pthread.h>
typedef struct thread_t {
   pthread_t   pth;
   pthread_attr_t *pa;
   int         refcnt;                 /* Reference count */
} thread_t;

extern thread_t *thr_create(void *cb, void *arg, size_t stack_size);
