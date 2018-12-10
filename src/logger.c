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
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "conf.h"
#include "logger.h"
#include "memory.h"
#include "util.h"

#if	!defined(LOG_EMERG)
#warning "Your sylog.h is broken... hackery ensues..."
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */
#endif	// !defined(LOG_EMERG)
struct log_levels {
   char *str;
   int level;
};

static struct log_levels log_levels[] = {
   { "debug", LOG_DEBUG },
   { "info", LOG_INFO },
   { "notice", LOG_NOTICE },
   { "warn", LOG_WARNING },
   { "error", LOG_ERR },
   { "crit", LOG_CRIT },
   { "alert", LOG_ALERT },
   { "emerg", LOG_EMERG },
   { NULL, -1 },
};

// Internal thread-local state keeping
typedef struct {
  enum { NONE = 0, SYSLOG, STDOUT, LOGFILE, FIFO } type;
  FILE *fp;
} LogHndl;
static LogHndl *mainlog;

// Convert numeric log level to string
static inline const char *LogName(int level) {
   struct log_levels *lp = log_levels;

   do {
      if (lp->level == level)
         return lp->str;

      lp++;
   } while(lp->str != NULL);

   return NULL;
}

// Convert string log level to integer
static inline const int LogLevel(const char *name) {
   struct log_levels *lp = log_levels;

   do {
      if (strcmp(lp->str, name) == 0)
         return lp->level;
      lp++;
   } while(lp->str != NULL);
   return -1;
}

void Log(int level, const char *msg, ...) {
   va_list ap;
   char buf[4096];
   time_t t;
   struct tm *lt;
   int max;
   FILE *fp = stdout;

   if (!msg)
      return;   

   va_start(ap, msg);
   t = time(NULL);

   if ((lt = localtime(&t)) == NULL) {
      perror("localtime");
      return;
   }

   // If configuration isn't up yet, all messages are info priority
   if (&conf != NULL && conf.dict != NULL)
      max = LogLevel(dconf_get_str("log.level", "info"));
   else
      max = LogLevel("info");

   if (max < level)
      return;

   if (mainlog) {
      if (mainlog->type == syslog)
         vsyslog(level, msg, ap);

      if (mainlog->type != stdout && mainlog->fp)
         fp = mainlog->fp;
   }

   vsnprintf(buf, sizeof(buf) - 1, msg, ap);
   fprintf(fp, "%d/%02d/%02d %02d:%02d:%02d %5s: %s\n",
      lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec,
      LogName(level), buf);

   if (fp != stdout) {
      printf("%s\n", buf);
      fflush(stdout);
   }

   va_end(ap);
}

void log_open(const char *target) {
   if (mainlog)
      mem_free(mainlog);

   mainlog = mem_alloc(sizeof(Log));

   if (strcasecmp(target, "syslog") == 0) {
      mainlog->type = SYSLOG;
      openlog("jailfs", LOG_NDELAY|LOG_PID, LOG_DAEMON);
   } else if (strcasecmp(target, "stdout") == 0) {
      mainlog->type = STDOUT;
      mainlog->fp = STDOUT;
   } else if (strncasecmp(target, "fifo://", 7) == 0) {
      if (is_fifo(target + 7) || is_file(target + 7))
         unlink(target + 7);

      mkfifo(target+7, 0600);

      if (!(mainlog->fp = fopen(target + 7, "w"))) {
         Log(LOG_ERR, "Failed opening log fifo '%s' %s (%d)", target+7, errno, strerror(errno));
         mainlog->fp = STDOUT;
      } else
         mainlog->type = FIFO;
   } else if (strncasecmp(target, "file://", 7) == 0) {
      if (!(mainlog->fp = fopen(target + 7, "w+"))) {
         Log(LOG_ERR, "failed opening log file '%s' %s (%d)", target+7, errno, strerror(errno));
         mainlog->fp = STDOUT;
      } else
         mainlog->type = LOGFILE;
   }
}


static void log_close(void) {
   if (mainlog == NULL)
      return;

   if (mainlog->type == LOGFILE || mainlog->type == FIFO)
      fclose(mainlog->fp);
   else if (mainlog->type == SYSLOG)
      closelog();

   mem_free(mainlog);
   mainlog = NULL;
}


void *thread_logger_init(void *data) {
   thread_entry((dict *)data);
   log_open(dconf_get_str("path.log", "file://jailfs.log"));

   // Logger should remain running even after conf.dying is set.
   // We will (hopefully) be the last thing to shut down before main thread exits.
   while(1) {
      // XXX: Check for new log events & dispatch as needed
      pthread_yield();
      sleep(3);
   }

   return NULL;
}

void *thread_logger_fini(void *data) {
   log_close();
   thread_exit((dict *)data);
   return NULL;
}
