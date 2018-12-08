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
#if	!defined(__LOGGER)
#define __LOGGER
#include <stdio.h>
#include <syslog.h>
#define	__logger_h
#include "conf.h"

// This is the ONLY public interface that should be used!
extern void Log(int priority, const char *fmt, ...);

// This is the logger thread, which actually writes out all log messages and
// sends them to the shell to be displayed
extern void *thread_logger_init(void *data);		// Startup
extern void *thread_logger_fini(void *data);		// Shutdown

#endif                                 /* !defined(__LOGGER) */
