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
#include <lsd.h>
#include "linenoise.h"
#include "logger.h"
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
#define	SHELL_PROMPT	"jailfs"

BlockHeap *shell_hints_heap = NULL;

// Shell prompt
static char shell_prompt[64];
static char shell_level[40];

// this is in src/kilo.c
void cmd_edit(int argc, char **argv) {
//   kilo_main(argv[1]);
   kilo_main("/test.c");
}

// Use the signal handler to properly shut don the system (SIGTERM/11)
void cmd_shutdown(int argc, char **argv) {
   Log(LOG_NOTICE, "shutdown command from console.");
   raise(SIGTERM);
}

static void cmd_user(int argc, char **argv) {
   printf("User error: replace user\nGoodbye!\n");
   raise(SIGTERM);
}

static void cmd_run(int argc, char **argv) {
   printf("Starting cell!");
}

// Prototype for help function
void cmd_help(int argc, char **argv);

// Clear the screen via linenoise
static void cmd_clear(int argc, char **argv) {
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
   printf("changing shell_level: %s\n", line);
   shell_prompt_reset();
}


static void cmd_back(int argc, char **argv) {
   shell_level_set("main");
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
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "false", "Set false", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "set", "Set value", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "show", "Show state", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "true", "Set true", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_conf[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "dump", "Dump config values", HINT_CYAN, 1, 0, 0, 0, cmd_conf_dump, NULL },
   { "load", "Load saved config file", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "save", "Write config file", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "set", "Set config value", HINT_CYAN, 1, 0, 3, 3, NULL, NULL },
   { "show", "Get config value", HINT_CYAN, 1, 0, 1, 3, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_cron[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_RED, 0, 1, 0, 1, NULL, menu_value },
   { "jobs", "Show scheduled events", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "stop", "Stop a scheduled event", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_db[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_RED, 0, 1, 0, 1, NULL, menu_value },
   { "dump", "Dump the database to .sql file", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "purge", "Re-initialize the database", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};


static struct shell_cmd menu_debug[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "symtab_lookup", "Lookup a symbol in the symtab", HINT_CYAN, 1, 0, 1, 2, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_vfs[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "cd", "Change directory", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "chown", "Change file/dir ownership in jail", HINT_CYAN, 1, 0, 2, -1, NULL, NULL },
   { "chmod", "Change file/dir permissions in jail", HINT_CYAN, 1, 0, 2, -1, NULL, NULL },
   { "cp", "Copy file in jail", HINT_CYAN, 1, 0, 1, -1, NULL, NULL },
   { "debug", "Show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "edit", "Edit the file in kilo", HINT_GREEN, 1, 0, 1, 1, cmd_edit, NULL },
   { "less", "Show contents of a file (with pager)", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "ls", "Display directory listing", HINT_CYAN, 1, 0, 0, 1, NULL, NULL },
   { "mv", "Move file/dir in jail", HINT_RED, 0, 0, 1, -1, NULL, NULL },
   { "rm", "Remove file/directory in jail", HINT_RED, 0, 0, 1, -1, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_logging[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem_gc[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "now", "Run garbage collection now", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_hooks[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_RED, 0, 1, 0, 1, NULL, menu_value },
   { "list", "List registered hooks", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { "unregister", "Unregister a hook", HINT_RED, 0, 0, 1, 1, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem_bh_tuning[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem_bh[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "tuning", "Tuning knobs", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem_bh_tuning },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_mem[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "blockheap", "BlockHeap allocator", HINT_CYAN, 1, 1, 0, -1, NULL, menu_mem_bh },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "gc", "Garbage collector", HINT_CYAN, 1, 1, 0, -1, cmd_help, menu_mem_gc },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_module[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_net[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_pkg[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "scan", "Scan package pool and add to database", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_profiling[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "enable", "show/toggle profiling status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "save", "Save profiling data to disk", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_thread[] = {
   { "back", "Go back to top-level", HINT_GREEN, 0, 0, 0, 0, cmd_back, NULL },
   { "debug", "show/toggle debugging status", HINT_CYAN, 1, 1, 0, 1, NULL, menu_value },
   { "kill", "Kill a thread", HINT_RED, 0, 0, 1, 1, NULL, NULL },
   { "show", "Show details about thread", HINT_CYAN, 1, 0, 1, 1, NULL, NULL },
   { "list", "List running threads", HINT_CYAN, 1, 0, 0, 0, NULL, NULL },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd menu_main[] = {
   { "clear", "Clear screen", HINT_CYAN, 1, 0, 0, 0, &cmd_clear, NULL },
   { "conf", "Configuration keys", HINT_CYAN, 1, 0, 0, -1, &cmd_help, NULL },
   { "cron", "Periodic event scheduler", HINT_CYAN, 1, 1, 0, -1, NULL, menu_cron },
   { "db", "Database admin", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_db },
   { "debug", "Built-in debugger", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_debug },
   { "help", "Display help", HINT_CYAN, 1, 0, 0, -1, &cmd_help, NULL },
   { "hooks", "Hooks management", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_hooks },
   { "logging", "Log file", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_logging },
   { "memory", "Memory manager", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_mem },
   { "module", "Loadable module support", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_module },
   { "net", "Network", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_net },
   { "pkg", "Package commands", HINT_CYAN, 1, 1, 1, -1, &cmd_help, menu_pkg },
   { "profiling", "Profiling support", HINT_CYAN, 1, 1, 0, -1, NULL, menu_profiling },
   { "quit", "Alias to shutdown", HINT_RED, 1, 0, 0, 0, &cmd_shutdown, NULL },
   { "reload", "Reload configuration", HINT_CYAN, 1, 0, 0, 0, &cmd_reload, NULL },
   { "run", "Start the container (if autorun disabled)", HINT_GREEN, 1, 0, 0, 0, &cmd_run, NULL },
   { "shutdown", "Terminate the service", HINT_RED, 1, 0, 0, 0, &cmd_shutdown, NULL },
   { "stats", "Display statistics", HINT_CYAN, 1, 0, 0, 0, &cmd_stats, NULL },
   { "thread", "Thread manager", HINT_CYAN, 1, 1, 0, -1, &cmd_help, menu_thread },
   { "user", "Tools for users", HINT_YELLOW, 0, 0, 0, 0, &cmd_user, NULL },
   { "vfs", "Virtual FileSystem (VFS)", HINT_CYAN, 1, 1, 1, -1, &cmd_help, menu_vfs },
   { .cmd = NULL, .desc = NULL, .menu = NULL },
};

static struct shell_cmd *shell_get_menu(const char *line) {
   struct shell_cmd *menu = NULL;
   int x = 0, i = 0;

   // Find the appropriate menu context
   if (strcmp(shell_level, "main") == 0) {
      menu = menu_main;
   } else {
      x = sizeof(menu_main) / sizeof(menu_main[0]);
      i = 0;
      do {
         if (&menu_main[i] == NULL || menu_main[i].cmd == NULL)
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
   char *args[32], *last_p = NULL, *p = NULL;
   char *tmp;
   struct shell_cmd *menu = NULL;
   int i = 0;

   if (line == NULL)
      return -1;

   tmp = mem_alloc(strlen(line));
   memset(tmp, 0, sizeof(tmp));
   memset(args, 0, sizeof(args));
   memcpy(tmp, args, sizeof(tmp));

   args[0] = dconf_get_str("jail.name", NULL);

   strtok(tmp, " ");
   while ((p = strtok(NULL, " ")) != NULL) {
      if (p != NULL) {
         i++;
         args[i] = p;
      } else
         break;
   }

   // Command that apply in all menus:
   if (strncasecmp(line, "help", 4) == 0) {
      cmd_help(i, args);
   } else if (strcasecmp(line, "conf dump") == 0) {
      cmd_conf_dump(i, args);
   } else if (strcasecmp(line, "gc now") == 0) {
      printf("gc: Freed %d objects", gc_all());
   } else if (strcasecmp(line, "back") == 0) {
      shell_level_set("main");
   } else if (strcasecmp(line, "shutdown") == 0 || strcasecmp(line, "quit") == 0) {
      cmd_shutdown(i, args);
   } else {	// Attempt to render the menu..
      menu = shell_get_menu(line);

      do {
         if (&menu[i] == NULL || menu[i].desc == NULL)
            break;

         // Does this entry match our command?
         if (strcasecmp(menu[i].cmd, line) == 0) {
            if (menu[i].menu != NULL) {
               if (menu[i].menu == menu_value) {
                  // XXX: Handle debug stanzas - We don't cd to the menu...
                  printf("XXX: Not yet implemented\n");
               } else {
                  shell_level_set(line);
               }
               return 0;
            } else if (menu[i].handler != NULL) {
               menu[i].handler(i, args);
               return 0;
            } else {
               printf("That option '%s %s' is not yet implemented...\n", shell_level, line);
               return 0;
            }
         }
         i++;
      } while(1);
      printf("Unknown command: %s\t- try help\n", line);
   }
   return 0;
}

static void shell_completion(const char *buf, linenoiseCompletions *lc) {
   int i = 0;
   struct shell_cmd *menu = shell_get_menu(shell_level);

   do {
      if (&menu[i] == NULL || menu[i].desc == NULL)
         break;

      if (strncasecmp(buf, menu[i].cmd, strlen(buf)) == 0)
         linenoiseAddCompletion(lc, menu[i].cmd);

      i++;
   } while(1);

   // Extras (until we clean this mess up...)
   if (strcasecmp(buf, "gc now") == 0)
      linenoiseAddCompletion(lc, "gc now");
   else if (strcasecmp(buf, "conf dump") == 0)
      linenoiseAddCompletion(lc, "conf dump");
}

static char *shell_hints(const char *buf, int *color, int *bold) {
   int i = 0;
   struct shell_cmd *menu = shell_get_menu(shell_level);
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
    blockheap_free(shell_hints_heap, buf);
}

void cmd_help(int argc, char **argv) {
   struct shell_cmd *menu = NULL;
   int i = 0;

//   if (argc == 0) {
//      printf("Using menu: main\n");
//      menu = menu_main;
//   } else
//      menu = shell_get_menu(argv[1]);

   menu = shell_get_menu(shell_level);

   printf("Menu %s help\n", shell_level);
   do {
      if (&menu[i] == NULL || menu[i].desc == NULL)
         break;

      printf("%15s\t - %s%s\n", menu[i].cmd, menu[i].desc, (menu[i].menu == NULL ? "" : " (menu)"));

      i++;
   } while(1);
   return;
}

//
// Initialize the shell/debugger thread
//
void *thread_shell_init(void *data) {
   char *line = NULL;

   thread_entry((dict *)data);
   shell_hints_heap = blockheap_create(SHELL_HINT_MAX, 32, "shell hints");

   // Configure the input widget appropriately
   linenoiseSetMultiLine(1);
   linenoiseSetCompletionCallback(shell_completion);
   linenoiseSetHintsCallback(shell_hints);
   linenoiseSetFreeHintsCallback((void *)shell_hints_free);
   linenoiseHistorySetMaxLen(dconf_get_int("shell.history-length", 100));
   linenoiseHistoryLoad("state/.shell.history");

   // Try to avoid our console prompt being spammed by startup text
   sleep(1);
   Log(LOG_INFO, "Ready to accept requests");
   sleep(1);
   printf("jailfs shell starting. You are managing jail '%s'\n\n", dconf_get_str("jail.name", NULL));
   printf("Try 'help' for a list of available commands or 'shutdown' to halt the service\nTab completion is enabled.\n\n");
   shell_level_set("main");

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
