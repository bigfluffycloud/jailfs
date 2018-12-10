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
#if	!defined(__SHELL_H)
#define	__SHELL_H

struct shell_cmd {
    const char *cmd;
    const char *desc;	// Description
    const int color;	// Color code (for linenoise)
    const int bold;	// bold text
    const int submenu;	// 0 for false, 1 for true
    const int min_args,
              max_args;
    void (*handler)();
    struct shell_cmd *menu;
};

// Set up & tear down the shell thread
extern void *thread_shell_init(void *data);
extern void *thread_shell_fini(void *data);

#endif	// !defined(__SHELL_H)
