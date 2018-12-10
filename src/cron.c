/*
 * tk/servers/fs-pkg:
 *	Package filesystem. Allows mounting various package files as
 *   a read-only (optionally with spillover file) overlay filesystem
 *   via FUSE or (eventually) LD_PRELOAD library.
 *
 * Copyright (C) 2012-2018 BigFluffy.Cloud <joseph@bigfluffy.cloud>
 *
 * Distributed under a MIT license. Send bugs/patches by email or
 * on github - https://github.com/bigfluffycloud/fs-pkg/
 *
 * No warranty of any kind. Good luck!
 *
 * src/cron.c:
 *	Periodic event handler support
 */
#include <time.h>
#include <stdlib.h>
#include "cron.h"
#include "conf.h"
#include "ev.h"
#include "memory.h"
#include "logger.h"
struct ev_loop *evt_loop = NULL;

void evt_init(void) {
   evt_loop = ev_default_loop(0);
}

ev_timer   *evt_timer_add_periodic(void *callback, const char *name, int interval) {
   ev_timer   *timer = mem_alloc(sizeof(ev_timer));
   ev_timer_init(timer, callback, 0, interval);
   ev_timer_start(evt_loop, timer);
   return timer;
}

/* MAKE DAMN SURE YOUR CALLBACK DOES A mem_free() ON TIMER!!! */
ev_timer   *evt_timer_add_oneshot(void *callback, const char *name, int timeout) {
   ev_timer   *timer = mem_alloc(sizeof(ev_timer));
   ev_timer_init(timer, callback, timeout, 0);
   ev_timer_start(evt_loop, timer);
   return timer;
}

// We need to keep things tight here as we run once every second!
static void cron_tick(int fd, short event, void *arg) {
   // Update the global time reference (reduces syscalls...)
   conf.now = time(NULL);

   // Garbage collect
   if (conf.now % (timestr_to_time(dconf_get_str("tuning.timer.global_gc", NULL), 60)) == 1) {
      // XXX: Store before stats.
      // Force garbage collection of all BlockHeaps
      gc_all();
      // XXX: Log before/after stats
   }

   // XXX: Determine if cron jobs are ready to run
   // XXX: Schedule them to be ran soon
   // XXX: Run pending jobs, if CPU load allows, else reschedule
}

int cron_init(void) {
    int rv = EXIT_FAILURE;

    // Call cron once per second to update conf.now and
    // schedule any pending jobs
    evt_timer_add_periodic(cron_tick, "tick", 1);
    Log(LOG_DEBUG, "starting periodic task scheduler (cron)");

    return rv;
}
