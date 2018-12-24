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
#if	!defined(__threads_h)
#define	__threads_h
#include <pthread.h>
#include <lsd/lsd.h>

typedef struct thread {
   pthread_t 	thr_info;
   pthread_attr_t thr_attr;
   int          refcnt;
   void         *(*fini)(void *);
   char		*argv;
} Thread;

typedef struct ThreadPool {
  char       *name;
  list_p      list;
  pthread_attr_t pth_attr;
} ThreadPool;

extern ThreadPool *threadpool_init(const char *name, const char *opts);
extern int threadpool_destroy(ThreadPool *pool);
extern Thread *thread_create(ThreadPool *pool, void *(*init)(void *), void *(*fini)(void *), void *arg, const char *descr);
extern Thread *thread_detach(ThreadPool *pool, Thread *thr);

extern void thread_entry(dict *_conf);
extern void thread_exit(dict *_conf);
extern int core_ready;
extern pthread_cond_t core_ready_c;
extern pthread_mutex_t core_ready_m;

#endif	/* !defined(__threads_h) */
