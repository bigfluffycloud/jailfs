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
#if	!defined(__LOGGER)
#define __LOGGER
#include <stdio.h>
#define	__logger_h
#include "conf.h"

#if	!defined(__LOGGER_FP)
extern FILE *log_fp;
#endif

extern FILE *log_open(const char *path);
extern void log_close(FILE * fp);
extern void Log(enum log_priority priority, const char *fmt, ...);
#endif                                 /* !defined(__LOGGER) */
