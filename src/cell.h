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

#if	!defined(__JAIL_H)
#define	__JAIL_H

extern int jail_drop_priv(void);
extern void *thread_cell_init(void *data);
extern void *thread_cell_fini(void *data);

#endif	// !defined(__JAIL_H)
