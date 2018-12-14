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
#if	!defined(__CRON_H)
#define	__CRON_H
#include <ev.h>
extern struct ev_loop *evt_loop;
extern	int	cron_init(void);
extern void evt_init(void);
extern ev_timer *evt_timer_add_periodic(void *callback, const char *name, int interval);
/* MAKE DAMN SURE YOUR CALLBACK DOES A mem_free() ON TIMER!!! */
extern ev_timer *evt_timer_add_oneshot(void *callback, const char *name, int timeout);

#endif	// !defined(__CRON_H)

