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
 *
 * shell.c: A linenoise based simple CLI interface
 *       which should help ease setup and debugging
 *
 *	It primarily exists to allow access to these things:
 *		* dconf (configuration file)
 *		* debug (debugger)
 *		* log (log message handler)
 */
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <lsd/lsd.h>
#include "linenoise.h"
#include "shell.h"
#include "unix.h"
#include "conf.h"
#include "threads.h"
#include "gc.h"

static BlockHeap *heap_shell_hints = NULL;
// extern from kilo.c
extern int kilo_main(const char *filename);

// Shell prompt
static char shell_prompt[64];
static char shell_level[40];

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
   { "shell", LOG_SHELL },
   { NULL, -1 },
};

// Internal thread-local state keeping
typedef struct {
  enum { NONE = 0, SYSLOG, STDOUT, LOGFILE, FIFO } type;
  FILE *fp;
} LogHndl;
static LogHndl *mainlog;

//////////////////////////////////////////////////////////
// Initial (early) log support, storing messages in memory until logfile is open

static char *logger_earlylog = NULL,
            *earlylog_p = NULL;

static void *earlylog_init(size_t bufsz) {
     if (logger_earlylog != NULL) {
        Log(LOG_DEBUG, "earlylog_init() called twice, why?");
        return logger_earlylog;
     }

     if ((logger_earlylog = mem_alloc(MAX_EARLYLOG)) != NULL)
        return logger_earlylog;

     return NULL;
}

static int earlylog_append(const char *line) {
    // XXX: Add to the end of the log
    if (earlylog_p != NULL) {
       // Append it
       Log(LOG_SHELL, "early-log: %s", line);
    }
    return 0;
}

static void earlylog_fini(void) {
    if (logger_earlylog != NULL) {
       memset(logger_earlylog, 0, MAX_EARLYLOG);
       mem_free(logger_earlylog);
    }
    logger_earlylog = NULL;
}
////////////////////////////////////////////
   
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
   int min_lvl;
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
   if (conf.dict != NULL)
      min_lvl = LogLevel(dconf_get_str("log.level", "info"));
   else
      min_lvl = LogLevel("info");

#if	0
   // If log level of message is lower than minimum level, ignore it
   if (min_lvl > level) {
      printf("min_lvl: %d <%s> || level: %d <%s>\n", min_lvl, LogLevel(min_lvl), level, LogLevel(level));
      return;
   }
#endif	// 0

   if (mainlog) {
      if (mainlog->type == SYSLOG)
         vsyslog(level, msg, ap);

      if (mainlog->type != STDOUT && mainlog->fp)
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
      mainlog->fp = stdout;
   } else if (strncasecmp(target, "fifo://", 7) == 0) {
      if (is_fifo(target + 7) || is_file(target + 7))
         unlink(target + 7);

      mkfifo(target+7, 0600);

      if (!(mainlog->fp = fopen(target + 7, "w"))) {
         Log(LOG_ERR, "Failed opening log fifo '%s' %s (%d)", target+7, errno, strerror(errno));
         mainlog->fp = stdout;
      } else
         mainlog->type = FIFO;
   } else if (strncasecmp(target, "file://", 7) == 0) {
      if (!(mainlog->fp = fopen(target + 7, "w+"))) {
         Log(LOG_ERR, "failed opening log file '%s' %s (%d)", target+7, errno, strerror(errno));
         mainlog->fp = stdout;
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

// this is in src/kilo.c
void cmd_edit(dict *args) {
   char *ac;

   if (args == NULL)
      return;

   ac = (char *)dict_get(args, "1", NULL);

   if (ac != NULL) {
      char path[PATH_MAX];
      memset(path, 0, sizeof(path));
      snprintf(path, sizeof(path) - 1, "./%s", ac);
      snprintf(path, sizeof(path) - 1, "%s/%s", ac, dconf_get_str("path.mountpoint", NULL));
      kilo_main(path);
   }
}

// Use the signal handler to properly shut down the system (SIGTERM/11)
void cmd_shutdown(dict *args) {
   Log(LOG_NOTICE, "shutdown command from console.");
   raise(SIGTERM);
}

static void cmd_user(dict *args) {
   Log(LOG_SHELL, ">> User tools <<");
}

static void cmd_run(dict *args) {
   Log(LOG_SHELL, "launching jail!");
}

// Prototype for help function
void cmd_help(dict *args);

// Clear the screen via linenoise
static void cmd_clear(dict *args) {
   linenoiseClearScreen();
   return;
}

/// Prompt stuff ///
static void shell_prompt_reset(void) {
   memset(shell_prompt, 0, sizeof(shell_prompt));
   snprintf(shell_prompt, sizeof(shell_prompt) - 1, "%s:%s> ", SHELL_PROMPT, shell_level);
}

static void shell_level_set(const char *line) {
   memset(shell_level, 0, sizeof(shell_level));
   snprintf(shell_level, sizeof(shell_level) - 1, "%s", line);
   shell_prompt_reset();
}


static void cmd_back(dict *args) {
   shell_level_set("main");
}

// Use the signal handler to trigger a config reload (SIGHUP/1)
void cmd_reload(dict *args) {
   raise(SIGHUP);
   return;
}

static void cmd_stats(dict *args) {
   Log(LOG_SHELL, "stats requested by user:");
}

static void cmd_conf_dump(dict *args) {
   Log(LOG_SHELL, "Dumping configuration:");
   dict_dump(conf.dict, stdout);
}

///////////
// Menus //
///////////
/* from shell.h
struct shell_cmd {
    const char *cmd;
    const char *desc;	// Description
    const int color;	// Color code (for linenoise)
    const int bold;	// bold text?
    const int submenu;	// 0 for false, 1 for true
    const int min_args,
              max_args;
    void (*handler)();
    struct shell_cmd *menu;
};
*/

#define	ArrayElements(x)		(sizeof(x) / sizeof(x[0]))

// Show/toggle (true/false)/set value
static struct shell_cmd menu_value[] = {
   { "false", "Set false", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "set", "Set value", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "show", "Show state", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "true", "Set true", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_conf[] = {
   { "dump", "Dump config values", HINT_CYAN, 1, 0, 0, 0, cmd_conf_dump, NULL },
   { "load", "Load saved config file", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "save", "Write config file", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "set", "Set config value", HINT_CYAN, 1, 0, 3, 3, NULL, NULL },
   { "show", "Get config value", HINT_CYAN, 1, 0, 1, 3, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_cron[] = {
   { "debug", "show/toggle debugging status", HINT_RED, 0, 1, 0, 1, NULL, menu_value },
   { "jobs", "Show scheduled events", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "stop", "Stop a scheduled event", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_db[] = {
   { "debug", "show/toggle debugging status", HINT_RED, 0, 1, 0, 1, NULL, menu_value },
   { "dump", "Dump the database to .sql file", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "purge", "Re-initialize the database", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};


static struct shell_cmd menu_debug[] = {
   { "symtab_lookup", "Lookup a symbol in the symtab", HINT_CYAN, 1, 0, 1, 2, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_vfs[] = {
   { "cd", "Change directory", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "chown", "Change file/dir ownership in jail", HINT_CYAN, 1, 0, 2, -1, NULL, NULL },
   { "chmod", "Change file/dir permissions in jail", HINT_CYAN, 1, 0, 2, -1, NULL, NULL },
   { "cp", "Copy file in jail", HINT_CYAN, 1, 0, 1, -1, NULL, NULL },
   { "debug", "Show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "edit", "Edit text file in kilo", HINT_GREEN, 1, 0, 1, 1, cmd_edit, NULL },
   { "less", "Show contents of a file (with pager)", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "ls", "Display directory listing", HINT_CYAN, 1, 0, 0, 1, NULL, NULL },
   { "mv", "Move file/dir in jail", HINT_RED, 0, 0, 1, -1, NULL, NULL },
   { "rm", "Remove file/directory in jail", HINT_RED, 0, 0, 1, -1, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_logging[] = {
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem_gc[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "now", "Run garbage collection now", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_hooks[] = {
   { "debug", "show/toggle debugging status", HINT_RED, 0, 1, 0, 1, NULL, menu_value },
   { "list", "List registered hooks", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "unregister", "Unregister a hook", HINT_RED, 0, 0, 1, 1, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem_bh_tuning[] = {
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem_bh[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "tuning", "Tuning knobs", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem_bh_tuning },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem[] = {
   { "blockheap", "BlockHeap allocator", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem_bh },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "gc", "Garbage collector", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem_gc },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_module[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_net[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_pkg[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "scan", "Scan package pool and add to database", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_profiling[] = {
   { "enable", "show/toggle profiling status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "save", "Save profiling data to disk", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_thread[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "kill", "Kill a thread", HINT_RED, 0, 0, 1, 1, NULL, NULL },
   { "show", "Show details about thread", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "list", "List running threads", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_main[] = {
   { "clear", "Clear screen", HINT_CYAN, 1, 0, 0, 0, cmd_clear, NULL },
   { "conf", "Configuration keys", HINT_CYAN, 1, 0, 0, -1, NULL, menu_conf },
   { "cron", "Periodic event scheduler", HINT_CYAN, 1, 1, 0, -1, NULL, menu_cron },
   { "db", "Database admin", HINT_CYAN, 1, 1, 0, -1, NULL, menu_db },
   { "debug", "Built-in debugger", HINT_CYAN, 1, 1, 0, -1, NULL, menu_debug },
   { "hooks", "Hooks management", HINT_CYAN, 1, 1, 0, -1, NULL, menu_hooks },
   { "logging", "Log file", HINT_CYAN, 1, 1, 0, -1, NULL, menu_logging },
   { "memory", "Memory manager", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem },
   { "module", "Loadable module support", HINT_CYAN, 1, 1, 0, -1, NULL, menu_module },
   { "net", "Network", HINT_CYAN, 1, 1, 0, -1, NULL, menu_net },
   { "pkg", "Package commands", HINT_CYAN, 1, 1, 1, -1, NULL, menu_pkg },
   { "profiling", "Profiling support", HINT_CYAN, 1, 1, 0, -1, NULL, menu_profiling },
   { "quit", "Alias to shutdown", HINT_RED, 1, 0, 0, 0, cmd_shutdown, NULL },
   { "reload", "Reload configuration", HINT_CYAN, 1, 0, 0, 0, cmd_reload, NULL },
   { "run", "Start the container (if autorun disabled)", HINT_GREEN, 1, 0, 0, 0, &cmd_run, NULL },
   { "shutdown", "Terminate the service", HINT_RED, 1, 0, 0, 0, cmd_shutdown, NULL },
   { "stats", "Display statistics", HINT_CYAN, 1, 0, 0, 0, cmd_stats, NULL },
   { "thread", "Thread manager", HINT_CYAN, 1, 1, 0, -1, NULL, menu_thread },
   { "user", "User manager", HINT_YELLOW, 0, 0, 0, 0, cmd_user, NULL },
   { "vfs", "Virtual FileSystem (VFS)", HINT_CYAN, 1, 1, 1, -1, NULL, menu_vfs },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd *shell_get_menu(const char *line) {
   struct shell_cmd *menu = NULL;
   int x = 0, i = 0;

   // Find the appropriate menu context
   if (shell_level == NULL)
      shell_level_set("main");

   if (strcmp(shell_level, "main") == 0) {
      menu = menu_main;
   } else {
      x = sizeof(menu_main) / sizeof(menu_main[0]);
      i = 0;
      do {
         if (&menu_main[i] == NULL || menu_main[i].menu)
            break;

         if (strcasecmp(menu_main[i].cmd, shell_level) == 0)
            return menu_main[i].menu;
         i++;
      } while(i < x);

      return NULL;
   }

   return menu;
}

static int shell_command(const char *line) {
   char *p = NULL, key[13];
   char *tmp;
   struct shell_cmd *menu = NULL;
   dict *args = dict_new();
   int i = 0;

   if (line == NULL)
      return -1;

   if (shell_level == NULL)
      shell_level_set("main");

   // Duplicate the (const) string given to us, so we can mangle it...
   tmp = mem_alloc(strlen(line));
   memset(tmp, 0, sizeof(*tmp));
   memcpy(tmp, args, sizeof(*tmp));

   if (tmp == NULL)
      return -1;

   strtok(tmp, " ");
   while ((p = strtok(NULL, " ")) != NULL) {
      if (p != NULL) {
         i++;
         memset(key, 0, sizeof(key));
         snprintf(key, sizeof(key) - 1, "%*d", 11, i);
         dict_add(args, key, p);
      } else
         break;
   }
   // Command that apply in all menus:
   if (strncasecmp(line, "help", 4) == 0) {
      cmd_help(args);
   } else if (strncasecmp(line, "vfs edit", 8) == 0) {
      if (*(line+8) != ' ') {
         Log(LOG_SHELL, "vfs edit requires an argument (file name)");
         return -1;
      }
      char path[PATH_MAX];
      memset(path, 0, sizeof(path));
      snprintf(path, sizeof(path) - 1, "%s/%s", line+9, dconf_get_str("path.mountpoint", NULL));
      kilo_main(path);
   } else if (strcasecmp(line, "back") == 0) {
      shell_level_set("main");
   } else if (strcasecmp(line, "shutdown") == 0 || strcasecmp(line, "quit") == 0) {
      cmd_shutdown(args);
   } else if (strcasecmp(line, "conf dump") == 0) {
      cmd_conf_dump(args);
   } else if (strcasecmp(line, "gc now") == 0) {
      Log(LOG_SHELL, "gc: Freed %d objects", gc_all());
   } else {	// Attempt to render the menu..
      menu = shell_get_menu(line);
      i = 0;

      while (1) {
         if (&menu[i] == NULL || menu[i].desc == NULL)
            break;

         // Does this entry match our command?
         if (strcasecmp(menu[i].cmd, line) == 0) {
            if (menu[i].menu != NULL) {
               if (menu[i].menu == menu_value) {
                  Log(LOG_SHELL, "XXX: Not yet implemented");
               } else {
                  shell_level_set(line);
               }
               return 0;
            } else if (menu[i].handler != NULL) {
               menu[i].handler(args);
               mem_free(tmp);
               return 0;
            } else {
               Log(LOG_SHELL, "That option '%s %s' is not yet implemented...", shell_level, line);
               mem_free(tmp);
               return 0;
            }
         }
         i++;
      }
      Log(LOG_SHELL, "Unknown command, try help: %s", line);
   }
   mem_free(tmp);
   return 0;
}

static void shell_completion(const char *buf, linenoiseCompletions *lc) {
   int i = 0;
   char tmp[128];
   struct shell_cmd *menu = shell_get_menu(shell_level);

// XXX: Something's fucky here... We'll get around to it eventually...
#if	1
   while (&menu[i] != NULL) {
      if (strncasecmp(buf, menu[i].cmd, strlen(buf)) == 0) {
         linenoiseAddCompletion(lc, menu[i].cmd);

         if (menu[i].menu != NULL) {
            struct shell_cmd *submenu = menu[i].menu;

            Log(LOG_SHELL, "---------------------");

            if (submenu != NULL) {
               Log(LOG_SHELL, "->");

               int j = 0;
               while (&submenu[j] != NULL) {
                  memset(tmp, 0, sizeof(tmp));
                  snprintf(tmp, sizeof(tmp) - 1, "%s %s", shell_level, submenu[i].cmd);
                  linenoiseAddCompletion(lc, tmp);
                  j++;
               }
               Log(LOG_SHELL, ".");
            }
         }
      }
      i++;
   }
#endif

   // Extras (until we clean this mess up...)
   if (strncasecmp(buf, "gc now", strlen(buf)) == 0)
      linenoiseAddCompletion(lc, "gc now");
   else if (strncasecmp(buf, "conf dump", strlen(buf)) == 0)
      linenoiseAddCompletion(lc, "conf dump");
   else if (strncasecmp(buf, "back", strlen(buf)) == 0)
      linenoiseAddCompletion(lc, "back");
   else if (strncasecmp(buf, "help", strlen(buf)) == 0)
      linenoiseAddCompletion(lc, "help");
   else if (strncasecmp(buf, "vfs ", strlen(buf)) == 0) {
      linenoiseAddCompletion(lc, "vfs");
      linenoiseAddCompletion(lc, "vfs edit");
   }
}

static char *shell_hints(const char *buf, int *color, int *bold) {
   int i = 0;
   struct shell_cmd *menu = shell_get_menu(shell_level);
   char *msg;

   if (strlen(buf) < 2)
      return NULL;

   msg = blockheap_alloc(heap_shell_hints);
   memset(msg, 0, SHELL_HINT_MAX);

   // Static entries that will soon go away...
   if (strncasecmp(buf, "gc now", 6) == 0) {
      *color = HINT_YELLOW;
      *bold = 1;
      snprintf(msg, SHELL_HINT_MAX, " - run garbage collector now");
      return msg;
   } else if (strncasecmp(buf, "conf dump", 9) == 0) {
      *color = HINT_YELLOW;
      *bold = 1;
      snprintf(msg, SHELL_HINT_MAX, " - dump active configuration file");
      return msg;
   } else if (strncasecmp(buf, "back", 4) == 0) {
      *color = HINT_GREEN;
      *bold = 1;
      snprintf(msg, SHELL_HINT_MAX, " - go to previous menu");
      return msg;
   } else if (strncasecmp(buf, "help", 4) == 0) {
      *color = HINT_GREEN;
      *bold = 1;
      snprintf(msg, SHELL_HINT_MAX, " - display help text");
      return msg;
   } else if (strncasecmp(buf, "vfs edit", 8) == 0) {
      *color = HINT_YELLOW;
      *bold = 1;
      snprintf(msg, SHELL_HINT_MAX, " - edit text file");
      return msg;
   } else {
      do {
         if (&menu[i] == NULL || menu[i].desc == NULL)
            break;

         if (strncasecmp(menu[i].cmd, buf, strlen(menu[i].cmd)) == 0) {
            *color = menu[i].color;
            *bold = menu[i].bold;
            snprintf(msg, SHELL_HINT_MAX, " - %s", menu[i].desc);
            return msg;
         }
         i++;
      } while(1);
   }
   return NULL;
}

static void shell_hints_free(char *buf) {
    blockheap_free(heap_shell_hints, buf);
}

void cmd_help(dict *args) {
   struct shell_cmd *menu = NULL;
   int i = 0;

   menu = shell_get_menu(shell_level);
   Log(LOG_SHELL, "Menu %s help", shell_level);

   if (menu != menu_main)
      Log(LOG_SHELL, "%15s\t - go back to previous menu", "back");

   Log(LOG_SHELL, "%15s\t - display help text", "help");

   do {
      if (&menu[i] == NULL || menu[i].desc == NULL)
         break;

      Log(LOG_SHELL, "%15s\t - %s%s", menu[i].cmd, menu[i].desc, (menu[i].menu == NULL ? "" : " (menu)"));

      i++;
   } while(1);
   return;
}

// Initialize the shell/debugger thread
void *thread_shell_init(void *data) {
   char *line = NULL;
   thread_entry((dict *)data);
   heap_shell_hints = blockheap_create(SHELL_HINT_MAX, 32, "shell hints");

   // Configure the input widget appropriately
   linenoiseSetMultiLine(0);
   linenoiseSetCompletionCallback(shell_completion);
   linenoiseSetHintsCallback(shell_hints);
   linenoiseSetFreeHintsCallback((void *)shell_hints_free);
   linenoiseHistorySetMaxLen(dconf_get_int("shell.history-length", 100));
   linenoiseHistoryLoad("state/.shell.history");

   // XXX: We need to properly lock rather than sleep!
   sleep(2);
   Log(LOG_INFO, "Ready to accept requests");
   Log(LOG_SHELL, "jailfs shell starting. You are managing jail '%s'", dconf_get_str("jail.name", NULL));
   Log(LOG_SHELL, "Try 'help' for a list of available commands or 'shutdown' to halt the service\nTab completion is enabled.");
   shell_level_set("main");

   // Support console resizing
   struct winsize w;
   int	lines = -1, columns = -1;

   // As long as the dying flag has not been set, we continue to run
   while (!conf.dying) {
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      lines = w.ws_row;
      columns = w.ws_col;
      printf("[%d;0[K", lines-1);
      line = linenoise(shell_prompt);				// Show prompt

      if (line == NULL)
         continue;

      // Shell commands (local) start with / and are not sent to main process
      if (line[0] != '\0') {
         // Dispatch it to the command interpreter
         if (shell_command(line) != -1) {
            linenoiseHistoryAdd(line);				// Add to history
            linenoiseHistorySave("state/.shell.history");	// Save history
         }
      }
      free(line);
   }
   return NULL;
}

// Shell destructor
void *thread_shell_fini(void *data) {
   linenoiseHistorySave("state/.shell.history");	// Save history
   log_close();
   blockheap_destroy(heap_shell_hints);
   thread_exit((dict *)data);
   return NULL;
}

int shell_gc(void) {
   blockheap_garbagecollect(heap_shell_hints);
   return 0;
}
