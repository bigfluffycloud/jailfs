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
#include "logger.h"
#include "linenoise.h"
#include "shell.h"
#include "unix.h"
#include "conf.h"
#include "threads.h"

/* linenoie colour codes:
       red = 31
       green = 32
       yellow = 33
       blue = 34
       magenta = 35
       cyan = 36
       white = 37;
 */

static const char *shell_prompt = "jailfs> ";

// Use the signal handler to properly shut don the system (SIGTERM/11)
void cmd_shutdown(int argc, char *argv) {
   Log(LOG_NOTICE, "shutdown command from console.");
   raise(SIGTERM);
   return;
}

// Prototype for help function
void cmd_help(int argc, char **argv);

// Clear the screen via linenoise
void cmd_clear(int argc, char **argv) {
   linenoiseClearScreen();
   return;
}

// Use the signal handler to trigger a config reload (SIGHUP/1)
void cmd_reload(int argc, char **argv) {
   raise(SIGHUP);
   return;
}

void cmd_stats(int argc, char **argv) {
   printf("stats requested by user:\n");
}

void cmd_conf_dump(int argc, char **argv) {
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
struct shell_cmd menu_value[] = {
   { "false", "Set false", 36, 1, 0, 0, 0, NULL, NULL },
   { "set", "Set value", 36, 1, 0, 1, 1, NULL, NULL },
   { "show", "Show state", 36, 1, 0, 0, 0, NULL, NULL },
   { "true", "Set true", 36, 1, 0, 0, 0, NULL, NULL }
};

struct shell_cmd conf_menu[] = {
   { "dump", "Dump config values", 36, 1, 0, 0, 0, cmd_conf_dump, NULL },
   { "load", "Load saved config file", 36, 1, 0, 1, 1, NULL, NULL },
   { "save", "Write config file", 36, 1, 0, 1, 1, NULL, NULL },
   { "set", "Set config value", 36, 1, 0, 3, 3, NULL, NULL },
   { "show", "Get config value", 36, 1, 0, 1, 3, NULL, NULL }
};

struct shell_cmd cron_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "jobs", "Show scheduled events", 36, 1, 0, 0, 0, NULL, NULL },
   { "stop", "Stop a scheduled event", 36, 1, 0, 1, 1, NULL, NULL }
};

struct shell_cmd db_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "dump", "Dump the database to .sql file", 36, 1, 0, 0, 0, NULL, NULL },
   { "purge", "Re-initialize the database", 36, 1, 0, 0, 0, NULL, NULL }
};


struct shell_cmd debug_menu[] = {
   { "symtab_lookup", "Lookup a symbol in the symtab", 36, 1, 0, 1, 2, NULL, NULL },
};

struct shell_cmd logging_menu[] = {
};

struct shell_cmd mem_gc_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "now", "Run garbage collection now", 36, 1, 0, 0, 0, NULL, NULL }
};

struct shell_cmd hooks_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "list", "List registered hooks", 36, 1, 0, 0, 0, NULL, NULL },
   { "unregister", "Unregister a hook", 36, 1, 0, 1, 1, NULL, NULL }
};

struct shell_cmd mem_bh_tuning_menu[] = {
};

struct shell_cmd mem_bh_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "tuning", "Tuning knobs", 36, 1, 1, 0, -1, NULL, mem_bh_tuning_menu }
};

struct shell_cmd mem_menu[] = {
   { "blockheap", "BlockHeap allocator", 36, 1, 1, 0, -1, NULL, mem_bh_menu },
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "gc", "Garbage collector", 36, 1, 1, 0, -1, cmd_help, mem_gc_menu }
};

struct shell_cmd module_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value }
};

struct shell_cmd net_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value }
};

struct shell_cmd pkg_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "scan", "Scan package pool and add to database", 36, 1, 0, 0, 0, NULL, NULL }
};

struct shell_cmd profiling_menu[] = {
   { "enable", "show/toggle profiling status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "save", "Save profiling data to disk", 36, 1, 0, 0, 0, NULL, NULL }
};

struct shell_cmd thread_menu[] = {
   { "debug", "show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value },
   { "kill", "Kill a thread", 36, 1, 0, 1, 1, NULL, NULL },
   { "show", "Show details about thread", 36, 1, 0, 1, 1, NULL, NULL },
   { "list", "List running threads", 36, 1, 0, 0, 0, NULL, NULL }
};

struct shell_cmd vfs_menu[] = {
   { "debug", "Show/toggle debugging status", 36, 1, 1, 0, 1, NULL, menu_value }
};

struct shell_cmd main_menu[] = {
   { "cd", "Change directory", 36, 1, 0, 1, 1, NULL, NULL },
   { "chown", "Change file/dir ownership in jail", 36, 1, 0, 2, -1, NULL, NULL },
   { "chmod", "Change file/dir permissions in jail", 36, 1, 0, 2, -1, NULL, NULL },
   { "clear", "Clear screen", 36, 1, 0, 0, 0, &cmd_clear, NULL },
   { "conf", "Configuration keys", 36, 1, 0, 0, -1, &cmd_help, NULL },
   { "cp", "Copy file in jail", 36, 1, 0, 1, -1, NULL, NULL },
   { "cron", "Periodic event scheduler", 36, 1, 1, 0, -1, NULL, cron_menu },
   { "db", "Database admin", 36, 1, 1, 0, -1, &cmd_help, db_menu },
   { "debug", "Built-in debugger", 36, 1, 1, 0, -1, &cmd_help, debug_menu },
   { "help", "Display help", 36, 1, 0, 0, -1, &cmd_help, NULL },
   { "hooks", "Hooks management", 36, 1, 1, 0, -1, &cmd_help, hooks_menu },
   { "less", "Show contents of a file (with pager)", 36, 1, 0, 1, 1, NULL, NULL },
   { "logging", "Log file", 36, 1, 1, 0, -1, &cmd_help, logging_menu },
   { "ls", "Display directory listing", 36, 1, 0, 0, 1, NULL, NULL },
   { "memory", "Memory manager", 36, 1, 1, 0, -1, &cmd_help, mem_menu },
   { "module", "Loadable module support", 36, 1, 1, 0, -1, &cmd_help, module_menu },
   { "mv", "Move file/dir in jail", 36, 1, 0, 1, -1, NULL, NULL },
   { "net", "Network", 36, 1, 1, 0, -1, &cmd_help, net_menu },
   { "pkg", "Package commands", 36, 1, 1, 1, -1, &cmd_help, pkg_menu },
   { "profiling", "Profiling support", 36, 1, 1, 0, -1, NULL, profiling_menu },
   { "reload", "Reload configuration", 36, 1, 0, 0, 0, &cmd_reload, NULL },
   { "rm", "Remove file/directory in jail", 36, 1, 0, 1, -1, NULL, NULL },
   { "thread", "Thread manager", 36, 1, 1, 0, -1, &cmd_help, thread_menu },
   { "shutdown", "Terminate the service", 31, 1, 0, 0, 0, &cmd_shutdown, NULL },
   { "stats", "Display statistics", 36, 1, 0, 0, 0, &cmd_stats, NULL },
   { "vfs", "Virtual FileSystem (VFS)", 36, 1, 1, 1, -1, &cmd_help, vfs_menu },
   { "quit", NULL, 0, 0, 0, 0, 0, &cmd_shutdown, NULL }						// Unlisted alias to shutdown
};

int shell_command(const char *line) {
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
   // We need to break up the command line here into args & i
   if (strncasecmp(line, "help", 4) == 0) {
      cmd_help(i, &args);
   } else if (strcasecmp(line, "shutdown") == 0 || strcasecmp(line, "quit") == 0) {
      cmd_shutdown(i, &args);
   } else if (strcasecmp(line, "user") == 0) {
      printf("User error: replace user\nGoodbye!\n");
      cmd_shutdown(i, &args);
   } else if (strcasecmp(line, "conf dump") == 0) {
      cmd_conf_dump(i, &args);
   }

   return 0;
}

void shell_completion(const char *buf, linenoiseCompletions *lc) {
   // Scan through the menu and generate completions...
   if (buf[0] == 'h') {
      linenoiseAddCompletion(lc, "help");
   } else if (strncasecmp(buf, "shut", 4) == 0) {
      linenoiseAddCompletion(lc, "shutdown");
   }
}

char *shell_hints(const char *buf, int *color, int *bold) {
   // Scan through the menu and provide hints...
   if (!strcasecmp(buf, "help")) {
      *color = 36;
      *bold = 1;
      return " show help messages";
   } else if (!strcasecmp(buf, "shutdown") || !strcasecmp(buf, "quit")) {
      *color = 31;
      *bold = 1;
      return " terminate the jail";
   }

   return NULL;
}


void cmd_help(int argc, char **argv) {
   struct shell_cmd *menu = NULL;
   int i = 0;
   int x = 0;

   if (argc == 0) {
      printf("Using menu: main\n");
      menu = &main_menu;
   }
/*
    else {
      x = sizeof(main_menu) / sizeof(main_menu[0]);
      do {
         if (&main_menu[i] == NULL || &main_menu[i].menu == NULL) {
            printf("cmd_help: Skipping menu entry # %d - scanning submenus\n", i);
            break;
         }

         printf("submenu: %d<%s>: trying to match for %s\n", i, main_menu[i].cmd, argv[1]);
         if (strcasecmp(main_menu[i].cmd, argv[1]) == 0) {
            // Set our pointer to the submenu that matched
            menu = main_menu[i].menu;
            break;
         }
      } while (i < x);
      printf("Found %d entries in menu\n", x);
      printf("submenu scan end\n");
   }
*/
   x = sizeof(main_menu) / sizeof(main_menu[0]);
   i = 0;
   do {
      if (&main_menu[i] == NULL || main_menu[i].desc == NULL)
         break;

      printf("%15s\t - %s\n", main_menu[i].cmd, main_menu[i].desc);

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

   // Configure the input widget appropriately
   linenoiseSetMultiLine(1);
   linenoiseSetCompletionCallback(shell_completion);
   linenoiseSetHintsCallback(shell_hints);
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

   thread_exit((dict *)data);
   return NULL;
}
