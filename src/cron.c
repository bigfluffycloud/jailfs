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
#include <time.h>
#include "evt.h"
#include "cron.h"

// We need to keep things tight here as we run once every second!
static void cron_tick(int fd, short event, void *arg) {
   // Update the global time reference (reduces syscalls...)
   conf.now = time(NULL);

   // XXX: Determine if cron jobs are ready to run
   // XXX: Schedule them to be ran soon
   // XXX: Run pending jobs, if CPU load allows
}

int cron_init(void) {
    int rv = EXIT_FAILURE;

    // Call cron once per second to update conf.now and
    // schedule any pending jobs
    evt_timer_add_periodic(cron_tick, "tick", 1);
    return rv;
}
