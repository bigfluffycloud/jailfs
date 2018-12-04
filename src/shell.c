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
 */
#include <signal.h>
#include "logger.h"
#include "linenoise.h"

static const char *prompt = "jailfs> ";
struct shell_cmd {
    const char *cmd;
    const char *desc;	// Description
    const int submenu;	// 0 for false, 1 for true
    const int min_args,
              max_args;
    void (*handler)();
    struct shell_cmd *menu;
};

void cmd_shutdown(int argc, char **argv) {
   raise(SIGTERM);
   return;
}

void cmd_help(int argc, char **argv) {
   // We need to figure out which help text to display...
   return;
}

void cmd_reload(int argc, char **argv) {
   raise(SIGHUP);
   return;
}

void cmd_stats(int argc, char **argv) {
   Log(LOG_INFO, "stats requested by user: ");
}

// Show/toggle (true/false)/set value
struct shell_cmd menu_value[] = {
   { "false", "Set false", 0, 0, 0, NULL, NULL },
   { "set", "Set value", 0, 1, 1, NULL, NULL },
   { "show", "Show state", 0, 0, 0, NULL, NULL },
   { "true", "Set true", 0, 0, 0, NULL, NULL }
};

struct shell_cmd conf_menu[] = {
   { "list", "List config values", 0, 0, 0, NULL, NULL },
   { "load", "Load saved config file", 0, 1, 1, NULL, NULL },
   { "save", "Write config file", 0, 1, 1, NULL, NULL },
   { "set", "Set config value", 0, 3, 3, NULL, NULL },
   { "show", "Get config value", 0, 1, 3, NULL, NULL },
};

struct shell_cmd cron_menu[] = {
   { "jobs", "Show scheduled events", 0, 0, 0, NULL, NULL },
   { "stop", "Stop a scheduled event", 0, 1, 1, NULL, NULL }
};

struct shell_cmd db_menu[] = {
   { "debug", "show/toggle debugging status", 1, 0, 1, NULL, menu_value },
   { "dump", "Dump the database to .sql file", 0, 0, 0, NULL, NULL },
   { "purge", "Re-initialize the database", 0, 0, 0, NULL, NULL }
};


struct shell_cmd debug_menu[] = {
};

struct shell_cmd logging_menu[] = {
};

struct shell_cmd mem_gc_menu[] = {
   { "debug", "show/toggle debugging status", 1, 0, 1, NULL, menu_value },
   { "now", "Run garbage collection now", 0, 0, 0, NULL, NULL }
};

struct shell_cmd hooks_menu[] = {
   { "list", "List registered hooks", 0, 0, 0, NULL, NULL },
   { "unregister", "Unregister a hook", 0, 1, 1, NULL, NULL }
};

struct shell_cmd mem_bh_tuning_menu[] = {
};

struct shell_cmd mem_bh_menu[] = {
   { "tuning", "Tuning knobs", 1, 0, -1, NULL, mem_bh_tuning_menu }
};

struct shell_cmd mem_menu[] = {
   { "blockheap", "BlockHeap allocator", 1, 0, -1, NULL, mem_bh_menu },
   { "debug", "show/toggle debugging status", 1, 0, 1, NULL, menu_value },
   { "gc", "Garbage collector", 1, 0, -1, cmd_help, mem_gc_menu }
};

struct shell_cmd module_menu[] = {
   { "debug", "show/toggle debugging status", 1, 0, 1, NULL, menu_value }
};

struct shell_cmd net_menu[] = {
   { "debug", "show/toggle debugging status", 1, 0, 1, NULL, menu_value }
};

struct shell_cmd pkg_menu[] = {
   { "debug", "show/toggle debugging status", 1, 0, 1, NULL, menu_value },
   { "scan", "Scan package pool and add to database", 0, 0, 0, NULL, NULL }
};

struct shell_cmd profiling_menu[] = {
   { "enable", "show/toggle profiling status", 1, 0, 1, NULL, menu_value },
   { "save", "Save profiling data to disk", 0, 0, 0, NULL, NULL }
};

struct shell_cmd thread_menu[] = {
   { "debug", "show/toggle debugging status", 1, 0, 1, NULL, menu_value },
   { "kill", "Kill a thread", 0, 1, 1, NULL, NULL },
   { "show", "Show details about thread", 0, 1, 1, NULL, NULL },
   { "list", "List running threads", 0, 0, 0, NULL, NULL }
};

struct shell_cmd vfs_menu[] = {
   { "debug", "Show/toggle debugging status", 1, 0, 1, NULL, menu_value }
};

struct shell_cmd menu[] = {
   { "cd", "Change directory", 0, 1, 1, NULL, NULL },
   { "chown", "Change file/dir ownership in jail", 0, 2, -1, NULL, NULL },
   { "chmod", "Change file/dir permissions in jail", 0, 2, -1, NULL, NULL },
   { "conf", "Configuration keys", 0, 0, -1, &cmd_help, NULL },
   { "cp", "Copy file in jail", 0, 1, -1, NULL, NULL },
   { "cron", "Periodic event scheduler", 1, 0, -1, NULL, cron_menu },
   { "db", "Database admin", 1, 0, -1, &cmd_help, db_menu },
   { "debug", "Built-in debugger", 1, 0, -1, &cmd_help, debug_menu },
   { "help", "Display help", 0, 0, -1, &cmd_help, NULL },
   { "hooks", "Hooks management", 1, 0, -1, &cmd_help, hooks_menu },
   { "less", "Show contents of a file (with pager)", 0, 1, 1, NULL, NULL },
   { "logging", "Log file", 1, 0, -1, &cmd_help, logging_menu },
   { "ls", "Display directory listing", 0, 0, 1, NULL, NULL },
   { "memory", "Memory manager", 1, 0, -1, &cmd_help, mem_menu },
   { "module", "Loadable module support", 1, 0, -1, &cmd_help, module_menu },
   { "mv", "Move file/dir in jail", 0, 1, -1, NULL, NULL },
   { "net", "Network", 1, 0, -1, &cmd_help, net_menu },
   { "pkg", "Package commands", 1, 1, -1, &cmd_help, pkg_menu },
   { "profiling", "Profiling support", 1, 0, -1, NULL, profiling_menu },
   { "reload", "Reload configuration", 0, 0, 0, &cmd_reload, NULL },
   { "rm", "Remove file/directory in jail", 0, 1, -1, NULL, NULL },
   { "thread", "Thread manager", 1, 0, -1, &cmd_help, thread_menu },
   { "shutdown", "Terminate the service", 0, 0, 0, &cmd_shutdown, NULL },
   { "stats", "Display statistics", 0, 0, 0, &cmd_stats, NULL },
   { "vfs", "Virtual FileSystem (VFS)", 1, 1, -1, &cmd_help, vfs_menu } 
};
