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

#include <syslog.h>
#define	MAX_LOG_LINE	8192
#define	MAX_EARLYLOG	16384

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
#define	LOG_SHELL	65000	/* shell messages */
// Shell hints stuff
#define	HINT_RED	31
#define	HINT_GREEN	32
#define	HINT_YELLOW	33
#define	HINT_BLUE	34
#define	HINT_MAGENTA	35
#define	HINT_CYAN	36
#define	HINT_WHITE	37
#define	SHELL_HINT_MAX	120
#define	SHELL_PROMPT	"jailfs"
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

// This is the ONLY public interface that should be used!
extern void Log(int priority, const char *fmt, ...);

// Set up & tear down the shell thread
extern void *thread_shell_init(void *data);
extern void *thread_shell_fini(void *data);

#endif	// !defined(__SHELL_H)
