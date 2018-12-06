/* A fairly elegant logging system */
#include <ax/appworx.h>

LogHndl *mainlog = NULL;

struct log_levels {
   char *str;
   int level;
} log_levels[] = {
   { "debug", LOG_DEBUG },
   { "info", LOG_INFO },
   { "notice", LOG_NOTICE },
   { "warning", LOG_WARNING },
   { "error", LOG_ERR },
   { "critical", LOG_CRIT },
   { "alert", LOG_ALERT },
   { "emergency", LOG_EMERG },
   { NULL, -1 },
};

static inline const char *LogName(int level) {
   struct log_levels *lp = log_levels;

   do {
      if (lp->level == level)
         return lp->str;

      lp++;
   } while(lp->str != NULL);

   return LOG_INVALID_STR;
}

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
   FILE *fp = stderr;

   if (!msg)
      return;   

   va_start(ap, msg);
   t = time(NULL);

   if ((lt = localtime(&t)) == NULL) {
      perror("localtime");
      return;
   }

   max = LogLevel(dict_get(GConf, "log.level", "debug"));

   if (max < level)
      return;

//   if (mainlog) {
//      if (mainlog->type == LOG_syslog)
//         vsyslog(level, msg, ap);
//   }
//   if (mainlog->type != LOG_stderr && mainlog->fp)
//      fp = mainlog->fp;
   vsnprintf(buf, sizeof(buf) - 1, msg, ap);
   fprintf(fp, "%d/%02d/%02d %02d:%02d:%02d [%s] %s\n",
      lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec,
      LogName(level), buf);

   va_end(ap);
}

void log_open(const char *target) {
   if (mainlog)
      mem_free(mainlog);

   mainlog = mem_alloc(sizeof(Log));

   if (strcasecmp(target, "syslog") == 0) {
      mainlog->type = LOG_syslog;
      openlog("chatterd", LOG_NDELAY|LOG_PID, LOG_DAEMON);
   } else if (strcasecmp(target, "stderr") == 0) {
      mainlog->type = LOG_stderr;
      mainlog->fp = stderr;
   } else if (strncasecmp(target, "fifo://", 7) == 0) {
      if (is_fifo(target + 7) || is_file(target + 7))
         unlink(target + 7);

      mkfifo(target+7, 0600);

      if (!(mainlog->fp = fopen(target + 7, "w"))) {
         Log(LOG_ERR, "Failed opening log fifo '%s' %s (%d)", target+7, errno, strerror(errno));
         mainlog->fp = stderr;
      } else
         mainlog->type = LOG_fifo;
   } else if (strncasecmp(target, "file://", 7) == 0) {
      if (!(mainlog->fp = fopen(target + 7, "w+"))) {
         Log(LOG_ERR, "failed opening log file '%s' %s (%d)", target+7, errno, strerror(errno));
         mainlog->fp = stderr;
      } else
         mainlog->type = LOG_file;
   }
}

void log_close(void) {
   if (mainlog == NULL)
      return;

   if (mainlog->type == LOG_file || mainlog->type == LOG_fifo)
      fclose(mainlog->fp);
   else if (mainlog->type == LOG_syslog)
      closelog();

   mem_free(mainlog);
   mainlog = NULL;
}
