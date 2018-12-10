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
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "memory.h"
#include "balloc.h"
#include "logger.h"
#include "linenoise.h"
#include "shell.h"
#include "unix.h"
#include "conf.h"
#include "threads.h"
#include "gc.h"

//
// Shell hints stuff
//
#define	HINT_RED	31
#define	HINT_GREEN	32
#define	HINT_YELLOW	33
#define	HINT_BLUE	34
#define	HINT_MAGENTA	35
#define	HINT_CYAN	36
#define	HINT_WHITE	37

#define	SHELL_HINT_MAX	120
BlockHeap *shell_hints_heap = NULL;

// Shell prompt
static const char *shell_prompt = "jailfs> ";

// Use the signal handler to properly shut don the system (SIGTERM/11)
void cmd_shutdown(int argc, char *argv) {
   Log(LOG_NOTICE, "shutdown command from console.");
   raise(SIGTERM);
}

static void cmd_user(int argc, char *argv) {
   printf("User error: replace user\nGoodbye!\n");
   raise(SIGTERM);
}
// Prototype for help function
void cmd_help(int argc, char **argv);

// Clear the screen via linenoise
static void cmd_clear(int argc, char **argv) {
   linenoiseClearScreen();
   return;
}

// Use the signal handler to trigger a config reload (SIGHUP/1)
void cmd_reload(int argc, char **argv) {
   raise(SIGHUP);
   return;
}

static void cmd_stats(int argc, char **argv) {
   printf("stats requested by user:\n");
}

static void cmd_conf_dump(int argc, char **argv) {
   printf("Dumping configuration:\n");
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
   { "true", "Set true", HINT_CYAN, 1, 0, 0, 0, NULL, NULL }
};

static struct shell_cmd menu_conf[] = {
   { "dump", "Dump config values", HINT_CYAN, 1, 0, 0, 0, cmd_conf_dump, NULL },
   { "load", "Load saved config file", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "save", "Write config file", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "set", "Set config value", HINT_CYAN, 1, 0, 3, 3, NULL, NULL },
   { "show", "Get config value", HINT_CYAN, 1, 0, 1, 3, NULL, NULL }
};

static struct shell_cmd menu_cron[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "jobs", "Show scheduled events", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "stop", "Stop a scheduled event", HINT_CYAN, 1, 0, 1, 1, NULL, NULL }
};

static struct shell_cmd menu_db[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "dump", "Dump the database to .sql file", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "purge", "Re-initialize the database", HINT_CYAN, 1, 0, 0, 0, NULL, NULL }
};


static struct shell_cmd menu_debug[] = {
   { "symtab_lookup", "Lookup a symbol in the symtab", HINT_CYAN, 1, 0, 1, 2, NULL, NULL },
};

static struct shell_cmd menu_logging[] = {
};

static struct shell_cmd menu_mem_gc[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "now", "Run garbage collection now", HINT_CYAN, 1, 0, 0, 0, NULL, NULL }
};

static struct shell_cmd menu_hooks[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "list", "List registered hooks", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "unregister", "Unregister a hook", HINT_CYAN, 1, 0, 1, 1, NULL, NULL }
};

static struct shell_cmd menu_mem_bh_tuning[] = {
};

static struct shell_cmd menu_mem_bh[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "tuning", "Tuning knobs", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem_bh_tuning }
};

static struct shell_cmd menu_mem[] = {
   { "blockheap", "BlockHeap allocator", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem_bh },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "gc", "Garbage collector", HINT_CYAN, 1, 1, 0, -1, cmd_help, menu_mem_gc }
};

static struct shell_cmd menu_module[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value }
};

static struct shell_cmd menu_net[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value }
};

static struct shell_cmd menu_pkg[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "scan", "Scan package pool and add to database", HINT_CYAN, 1, 0, 0, 0, NULL, NULL }
};

static struct shell_cmd menu_profiling[] = {
   { "enable", "show/toggle profiling status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "save", "Save profiling data to disk", HINT_CYAN, 1, 0, 0, 0, NULL, NULL }
};

static struct shell_cmd menu_thread[] = {
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "kill", "Kill a thread", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "show", "Show details about thread", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "list", "List running threads", HINT_CYAN, 1, 0, 0, 0, NULL, NULL }
};

static struct shell_cmd menu_vfs[] = {
   { "debug", "Show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value }
};

static struct shell_cmd menu_main[] = {
   { "cd", "Change directory", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "chown", "Change file/dir ownership in jail", HINT_CYAN, 1, 0, 2, -1, NULL, NULL },
   { "chmod", "Change file/dir permissions in jail", HINT_CYAN, 1, 0, 2, -1, NULL, NULL },
   { "clear", "Clear screen", HINT_CYAN, 1, 0, 0, 0, &cmd_clear, NULL },
   { "conf", "Configuration keys", HINT_CYAN, 1, 0, 0, -1, &cmd_help, NULL },
   { "cp", "Copy file in jail", HINT_CYAN, 1, 0, 1, -1, NULL, NULL },
   { "cron", "Periodic event scheduler", HINT_CYAN, 1, 1, 0, -1, NULL, menu_cron },
   { "db", "Database admin", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_db },
   { "debug", "Built-in debugger", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_debug },
   { "help", "Display help", HINT_CYAN, 1, 0, 0, -1, &cmd_help, NULL },
   { "hooks", "Hooks management", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_hooks },
   { "less", "Show contents of a file (with pager)", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "logging", "Log file", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_logging },
   { "ls", "Display directory listing", HINT_CYAN, 1, 0, 0, 1, NULL, NULL },
   { "memory", "Memory manager", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_mem },
   { "module", "Loadable module support", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_module },
   { "mv", "Move file/dir in jail", HINT_CYAN, 1, 0, 1, -1, NULL, NULL },
   { "net", "Network", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_net },
   { "pkg", "Package commands", HINT_CYAN, 1, 1, 1, -1, &cmd_help, menu_pkg },
   { "profiling", "Profiling support", HINT_CYAN, 1, 1, 0, -1, NULL, menu_profiling },
   { "quit", "Alias to shutdown", HINT_RED, 1, 0, 0, 0, &cmd_shutdown, NULL },
   { "reload", "Reload configuration", HINT_CYAN, 1, 0, 0, 0, &cmd_reload, NULL },
   { "rm", "Remove file/directory in jail", HINT_CYAN, 1, 0, 1, -1, NULL, NULL },
   { "thread", "Thread manager", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_thread },
   { "shutdown", "Terminate the service", HINT_RED, 1, 0, 0, 0, &cmd_shutdown, NULL },
   { "stats", "Display statistics", HINT_CYAN, 1, 0, 0, 0, &cmd_stats, NULL },
   { "user", "Tools for users", HINT_YELLOW, 0, 0, 0, 0, &cmd_user, NULL },
   { "vfs", "Virtual FileSystem (VFS)", HINT_CYAN, 1, 1, 1, -1, &cmd_help, menu_vfs }
};

static int shell_command(const char *line) {
   char *args[32], *last_p = NULL, *p = NULL;
   char *tmp;
   int i = 0, x = 0;

   if (line == NULL)
      return -1;

   tmp = mem_alloc(strlen(line));
   memset(tmp, 0, sizeof(tmp));
   memset(args, 0, sizeof(args));
   memcpy(tmp, args, sizeof(tmp));

   args[0] = dconf_get_str("jailname", NULL);
/*
   strtok(tmp, " ");
   while ((p = strtok(NULL, " ")) != NULL) {
      if (p != NULL) {
         i++;
         args[i] = p;
      } else
         break;
   }
*/
   // We need to break up the command line here into args & i then use the menus ;)
   if (strncasecmp(line, "help", 4) == 0) {
      cmd_help(i, &args);
   } else if (strcasecmp(line, "shutdown") == 0 || strcasecmp(line, "quit") == 0) {
      cmd_shutdown(i, &args);
   } else if (strcasecmp(line, "user") == 0) {
      cmd_shutdown(i, &args);
   } else if (strcasecmp(line, "conf dump") == 0) {
      cmd_conf_dump(i, &args);
   } else if (strcasecmp(line, "gc now") == 0) {
      printf("gc: Freed %d objects", gc_all());
   } else
      printf("Unknown command: %s\t- try help\n", line);

   return 0;
}

static void shell_completion(const char *buf, linenoiseCompletions *lc) {
   int i = 0, x = 0;
   x = sizeof(menu_main) / sizeof(menu_main[0]);
   i = 0;

   do {
      if (&menu_main[i] == NULL || menu_main[i].desc == NULL)
         break;

      if (strncasecmp(buf, menu_main[i].cmd, strlen(buf)) == 0) {
         linenoiseAddCompletion(lc, menu_main[i].cmd);
         i++;
         continue;
      }
      i++;
   } while(i < x);

   // Extras (until we clean this mess up...)
   if (strcasecmp(buf, "gc now") == 0)
      linenoiseAddCompletion(lc, "gc now");
   else if (strcasecmp(buf, "conf dump") == 0)
      linenoiseAddCompletion(lc, "conf dump");
}

static char *shell_hints(const char *buf, int *color, int *bold) {
   int i = 0, x = 0;
   x = sizeof(menu_main) / sizeof(menu_main[0]);
   i = 0;
   char *msg;

   if (strlen(buf) < 2)
      return NULL;

   msg = blockheap_alloc(shell_hints_heap);
   memset(msg, 0, SHELL_HINT_MAX);


   // Static entries that will soon go away...
   if (strncasecmp(buf, "gc now", 6) == 0) {
      *color = HINT_YELLOW,
      *bold = 1,
      snprintf(msg, SHELL_HINT_MAX, " - run garbage collector now");
      return msg;
   } else if (strncasecmp(buf, "conf dump", 9) == 0) {
      *color = HINT_YELLOW,
      *bold = 1,
      snprintf(msg, SHELL_HINT_MAX, " - dump active configuration file");
      return msg;
   }

   // from menu_main etc
   do {
      if (&menu_main[i] == NULL || menu_main[i].desc == NULL)
         break;

      if (strncasecmp(menu_main[i].cmd, buf, strlen(menu_main[i].cmd)) == 0) {
         *color = menu_main[i].color;
         *bold = menu_main[i].bold;
         snprintf(msg, SHELL_HINT_MAX, " - %s", menu_main[i].desc);
         return msg;
      }
      i++;
   } while(i < x);

   return NULL;
}

static void shell_hints_free(const char *buf) {
    blockheap_free(shell_hints_heap, buf);
}

void cmd_help(int argc, char **argv) {
   struct shell_cmd *menu = NULL;
   int i = 0;
   int x = 0;

   if (argc == 0) {
      printf("Using menu: main\n");
      menu = &menu_main;
   }
/*
    else {
      x = sizeof(menu_main) / sizeof(menu_main[0]);
      do {
         if (&menu[i] == NULL || &menu_main[i].menu == NULL) {
            printf("cmd_help: Skipping menu entry # %d - scanning submenus\n", i);
            break;
         }

         printf("submenu: %d<%s>: trying to match for %s\n", i, menu_main[i].cmd, argv[1]);
         if (strcasecmp(menu_main[i].cmd, argv[1]) == 0) {
            // Set our pointer to the submenu that matched
            menu = menu_main[i].menu;
            break;
         }
      } while (i < x);
      printf("Found %d entries in menu\n", x);
      printf("submenu scan end\n");
   }
*/
   x = sizeof(menu_main) / sizeof(menu_main[0]);
   i = 0;
   do {
      if (&menu_main[i] == NULL || menu_main[i].desc == NULL)
         break;

      printf("%15s\t - %s\n", menu_main[i].cmd, menu_main[i].desc);

      i++;
   } while(i < x);
   return;
}

//
// Initialize the shell/debugger thread
//
void *thread_shell_init(void *data) {
   char *line;

   thread_entry((dict *)data);
   shell_hints_heap = blockheap_create(SHELL_HINT_MAX, 32, "shell hints");

   // Configure the input widget appropriately
   linenoiseSetMultiLine(1);
   linenoiseSetCompletionCallback(shell_completion);
   linenoiseSetHintsCallback(shell_hints);
   linenoiseSetFreeHintsCallback(shell_hints_free);
   linenoiseHistorySetMaxLen(dconf_get_int("shell.history-length", 100));
   linenoiseHistoryLoad("state/.shell.history");

   // Try to avoid our console prompt being spammed by startup text
   sleep(1);
   printf("jailfs shell starting. You are managing jail '%s'\n\n", dconf_get_str("jailname", NULL));
   printf("Try 'help' for a list of available commands or 'shutdown' to halt the service\nTab completion is enabled.\n\n");

   // As long as the dying flag has not been set, we continue to run
   while (!conf.dying) {
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
   blockheap_destroy(shell_hints_heap);
   thread_exit((dict *)data);
   return NULL;
}
