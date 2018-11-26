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

static void cron_self_patch(int fd, short event, void *arg) {
   Log(LOG_WARNING, "On-disk version of jailfs binary has changed... Since you've enabled experimental.patching, we'll try something fancy... Good luck, human!");
   Log(LOG_WARNING, "* creating patch set");
   Log(LOG_WARNING, "* suspending children...");
   Log(LOG_WARNING, "* patching children...");
   Log(LOG_WARNING, "* patching self...");
   Log(LOG_WARNING, "* resuming children...");
   Log(LOG_WARNING, "If you are reading this, we have not crashed and might actually still be (sorta) working!");
}

int cron_init(void) {
    int rv = EXIT_FAILURE;

    // Call cron once per second to update conf.now and
    // schedule any pending jobs
    evt_timer_add_periodic(cron_tick, "tick", 1);
    evt_timer_add_oneshot(cron_self_patch, "spd", 28);
    return rv;
}
