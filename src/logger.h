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
