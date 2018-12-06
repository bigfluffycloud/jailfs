#if	!defined(__threads_h)
#define	__threads_h
#include <pthread.h>
//#include "dictionary.h"
#include "dict.h"
#include "list.h"
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
