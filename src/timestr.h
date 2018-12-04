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
 */
#if	!defined(__TIMESTRING)
#define	__TIMESTRING
#include <time.h>
extern time_t timestr_to_time(const char *str, const time_t def);
extern char *time_to_timestr(time_t itime);
#endif                                 /* !defined(__TIMESTRING) */
