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
#include <signal.h>
#include "conf.h"
#include "evt.h"
#include "signal_handler.h"
static ev_signal signal_die;
static ev_signal signal_reload;

static void signal_handler_die(struct ev_loop *loop, ev_signal * w, int revents) {
   conf.dying = 1;
   goodbye();
}

static void signal_handler_reload(struct ev_loop *loop, ev_signal * w, int revents) {
}

void signal_init(void) {
   ev_signal_init(&signal_die, signal_handler_die, SIGINT);
   ev_signal_init(&signal_die, signal_handler_die, SIGQUIT);
   ev_signal_init(&signal_die, signal_handler_die, SIGTERM);
   ev_signal_init(&signal_reload, signal_handler_reload, SIGHUP);
   signal(SIGPIPE, SIG_IGN);
}
