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
#include <signal.h>
#include "logger.h"

struct shell_cmd {
    const char *cmd;
    const int submenu;	// 0 for false, 1 for true
    const int min_args,
              max_args;
    void (*ptr)();
};

void cmd_exit(int argc, char **argv) {
   raise(SIGTERM);
   return;
}

void cmd_help(int argc, char **argv) {
   return;
}

void cmd_reload(int argc, char **argv) {
   raise(SIGHUP);
   return;
}

void cmd_stats(int argc, char **argv) {
   Log(LOG_INFO, "stats requested by user: ");
}

struct shell_cmd cmds[] = {
    { "exit", 0, 0, 0, &cmd_exit },
    { "help", 0, 0, 0, &cmd_help },
    { "reload", 0, 0, 0, &cmd_reload },
    { "stats", 0, 0, 0, &cmd_stats }
};
