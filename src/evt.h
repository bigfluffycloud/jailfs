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
#include <ev.h>
#if	!defined(__EVT_H)
#define	__EVT_H

extern struct ev_loop *evt_loop;
extern void evt_init(void);
extern ev_timer *evt_timer_add_periodic(void *callback, const char *name, int interval);
/* MAKE DAMN SURE YOUR CALLBACK DOES A mem_free() ON TIMER!!! */
extern ev_timer *evt_timer_add_oneshot(void *callback, const char *name, int timeout);

#endif	// !defined(__EVT_H)
