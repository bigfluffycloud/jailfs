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
#if	!defined(__DCONF)
#define	__DCONF
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "dict.h"
#include "timestr.h"

enum log_priority {
   LOG_DEBUG = 0,
   LOG_INFO,
   LOG_HACK,
   LOG_WARNING,
   LOG_ERROR,
   LOG_FATAL
};

struct conf {
   FILE       *log_fp;
   enum log_priority log_level;
   char       *mountpoint;
   int         dying;
   dict *dict;
   time_t      born;
   time_t      now;
};

extern struct conf conf;

extern void dconf_init(const char *file);
extern void dconf_fini(void);
extern int  dconf_get_bool(const char *key, const int def);
extern double dconf_get_double(const char *key, const double def);
extern int  dconf_get_int(const char *key, const int def);
extern char *dconf_get_str(const char *key, const char *def);
extern time_t dconf_get_time(const char *key, const time_t def);
extern int  dconf_set(const char *key, const char *val);
extern void dconf_unset(const char *key);
#define	_DCONF_DICT conf.dict

#endif                                 /* !defined(__DCONF) */
