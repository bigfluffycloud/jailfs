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
/*
 * XXX: This needs finished, it works but its very incomplete...
 * XXX: we should support other forms of threading and maybe
 * XXX: even a fork() based model for things too small for
 * XXX: pthreads lib, anyone have a use for that?
 * XXX; - jld 20081102 [bug: 33]
 */
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "thread_pool.h"
#include "memory.h"
#include "logger.h"

thread_t   *thr_create(void *cb, void *arg, size_t stack_size) {
   thread_t   *ret = mem_alloc(sizeof(thread_t));

   if (!ret || (ret->pa = mem_alloc(sizeof(pthread_attr_t))) == NULL)
      raise(SIGTERM);

   if (pthread_attr_init(ret->pa) != 0) {
      Log(LOG_ERROR, "%s:pthread_attr_init %d:%s", __FUNCTION__, errno, strerror(errno));
      mem_free(ret);
      return NULL;
   }

   if (stack_size > 0)
      pthread_attr_setstacksize(ret->pa, stack_size);

   if (pthread_create(&ret->pth, ret->pa, cb, arg) != 0) {
      Log(LOG_ERROR, "%s:pthread_create %d:%s", __FUNCTION__, errno, strerror(errno));
      mem_free(ret);
      return NULL;
   }

   return ret;
}
