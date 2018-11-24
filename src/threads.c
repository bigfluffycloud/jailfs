/* Thread pools */
#include <pthread.h>
#include "dictionary.h"
#include "logger.h"
#include "threads.h"
int core_ready = 0;
pthread_mutex_t core_ready_m;
pthread_cond_t core_ready_c;

ThreadPool *threadpool_init(const char *name, const char *opts) {
  ThreadPool *r;
  int tmp;

  if (!(r = (ThreadPool *)mem_alloc(sizeof(ThreadPool)))) {
     Log(LOG_ERROR, "%s: allocation failed: [%d] %s", __FUNCTION__, errno, strerror(errno));
     return NULL;
  }

  if (pthread_attr_init(&(r->pth_attr)) != 0) {
     Log(LOG_FATAL, "%s: failed: %s (%d)", __FUNCTION__, strerror(errno), errno);
     return NULL;
  }

  if ((tmp = dict_getInt(conf.dict, "tuning.threads.stacksz", 0)) > 0) {
     if (pthread_attr_setstacksize(&(r->pth_attr), tmp) != 0)
        Log(LOG_ERROR, "%s: Unable to set stack size: %s (%d)", __FUNCTION__, strerror(errno), errno);
  }

  r->name = strdup(name);
  r->list = create_list();

  return r;
}

int threadpool_destroy(ThreadPool *pool) {
    return 0;
}

Thread *thread_create(ThreadPool *pool, void *(*init)(void *), void *(*fini)(void *), void *arg, const char *descr) {
  Thread *tmp = NULL;

  tmp = mem_alloc(sizeof(Thread));
  memset(tmp, 0, sizeof(Thread));
  memcpy((void *)&tmp->thr_attr, &(pool->pth_attr), sizeof(tmp->thr_attr));

  if (pthread_create(&tmp->thr_info, &tmp->thr_attr, init, &arg) != 0) {
     Log(LOG_ERROR, "thread creation failed: %s (%d)", strerror(errno), errno);
     return NULL;
  }

  /* Set pointer to destructor (if given) */
  if (fini)
     tmp->fini = fini;

  /* Add to thread pool */
  list_add(pool->list, tmp, sizeof(tmp));
  Log(LOG_DEBUG, "new thread %x created in pool %s", tmp, pool->name);
  return tmp;
}

Thread *thread_shutdown(ThreadPool *pool, Thread *thr) {
   if (!pool || !thr)
      return NULL;

//   list_remove(pool->list, thr);
   /* If reference count hits 0, destroy the thread */
   /* Theory: This should reduce some cache churn by keeping
    *   the fairly heavyweight worker threads from being restarted
    *   so often.
    * ToDo: This should do more error checking
    * Side-cases:
    */
   if (thr->refcnt == 0) {
      Log(LOG_DEBUG, "unallocating thread %x due to refcnt == 0", thr);

      if (thr->argv)
         mem_free(thr->argv);

      mem_free(thr);
      return NULL;
   }
   thr->refcnt--;

   return thr;
}

/*
 * You *MUST* call these at the beginning and end of your threads or bad things will happen,
 * like in-ability to access appworx services or crashes when you request them
 */
void thread_entry(dict *conf) {
    /* block until main thread is read */
    pthread_mutex_lock(&core_ready_m);
    while (core_ready < 1)
       pthread_cond_wait(&core_ready_c, &core_ready_m);
    pthread_mutex_unlock(&core_ready_m);

    /* Set up appworx runtime host OS dependant things */
    host_init();

    /* Configure AppWorx signals */
//    signal_init();

    if (GConf == NULL)
       GConf = conf;

    if (mainlog == NULL)
       mainlog = dict_get_blob(conf, "__mainlog__", NULL);
}

void thread_exit(dict *conf) {
    // XXX: See if this is old enough to exit
    pthread_exit(NULL);
}
