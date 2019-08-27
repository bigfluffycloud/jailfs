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
#if	!defined(__unix_h)
#define	__unix_h

extern void goodbye(void);             /* from main.c */
extern int daemon_restart(void);
extern void host_init(void);
extern int pidfile_open(const char *path);
extern void pidfile_close(void);

#endif	/* !defined(__unix_h) */
