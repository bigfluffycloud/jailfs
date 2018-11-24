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
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "conf.h"
#include "evt.h"
#include "signal_handler.h"
#include "logger.h"
#include "profiling.h"
int g_argc = -1;
char **g_argv = NULL;

/* src/module.c */
//extern int module_dying(int signal);

static ev_signal signal_die;
static ev_signal signal_reload;

static void signal_handler_die(struct ev_loop *loop, ev_signal * w, int revents) {
   conf.dying = 1;
   goodbye();
}

static void signal_handler_reload(struct ev_loop *loop, ev_signal * w, int revents) {
}

void signal_init_ev(void) {
   ev_signal_init(&signal_die, signal_handler_die, SIGINT);
   ev_signal_init(&signal_die, signal_handler_die, SIGQUIT);
   ev_signal_init(&signal_die, signal_handler_die, SIGTERM);
   ev_signal_init(&signal_reload, signal_handler_reload, SIGHUP);
   signal(SIGPIPE, SIG_IGN);
}

/*
 * Signal handling
 *	SIGHUP:	Reload configuration and flush caches/sync databases
 *	SIGUSR1: Dump profiling data
 *	SIGUSR2: Toggle profiling
 *	SIGTERM: Sync databases and gracefully terminate
 *      SIGSEGV: Restart module or daemon instead of crashing, dumping core if supported
 *	SIGKILL: Get into a consistent state and shut down
 *
 * Special signals
 *	SIGILL: Illegal instruction- debugger
 *	SIGTRAP: Trap into debugger
 */

static void signal_handler(int signal) {
   if (signal == SIGTERM || signal == SIGQUIT)
      exit(EXIT_FAILURE);

   if (signal == SIGSEGV) {
      stack_unwind();
#if	0
      if (in_module) {
         if (module_dying(signal) != 0)
            daemon_restart();
      } else
         conf.dying = 1;
#endif
//   } else if (signal == SIGUSR1) {
//      profiling_dump();
//   } else if (signal == SIGUSR2) {
//      profiling_toggle();
   }
   else if (signal == SIGCHLD) /* Prevent zombies */
      while(waitpid(-1, NULL, WNOHANG) > 0)
         ;
   Log(LOG_WARNING, "Caught signal %d", signal);
}

#if	0
struct SignalMapping {
   int signum;		/* Signal number */
   int flags;		/* See man 3 sigaction */
   int blocked;		/* Signals blocked during handling */
} SignalSet[] = {
   { SIGHUP, SA_RESTART, NULL }
   { SIGUSR1, SA_RESTART, SIGUSR1|SIGHUP },
   { SIGUSR2, SA_RESTART, SIGUSR2|SIGHUP },
   { SIGQUIT, SA_RESETHAND, SIGUSR1|SIGUSR2|SIGHUP },
   { SIGTERM, SA_RESTART, SIGUSR1|SIGUSR2|SIGHUP|SIGTERM },
   { SIGSEGV, SA_RESETHAND|SA_ONSTACK, SIGHUP|SIGSEGV },
   { SIGCHLD, SA_RESTART, SIGCHLD },
   { SIGILL, SA_RESTART, SIGILL|SIGSEGV|SIGTRAP },
   { SIGTRAP, SA_RESTART, SIGILL|SIGSEGV|SIGTRAP },
   { SIGPIPE, SA_RESTART, SIGPIPE|SIGCHLD },
   { -1, -1, -1 },
};
#endif // 0
static int signal_set[] = { SIGHUP, SIGUSR1, SIGUSR2, SIGQUIT, SIGTERM, SIGPIPE, SIGCHLD, SIGSEGV, SIGCHLD };

void signal_init(void) {
   struct sigaction action, old;
   int i = 0;

   Log(LOG_DEBUG, "setting up signal handlers");
   sigfillset(&action.sa_mask);
   action.sa_handler = signal_handler;

   do {
     if (signal_set[i]) {
        /* If SIGSEGV, switch to debugger stack */
        if (signal_set[i] == SIGSEGV)
           action.sa_flags = SA_ONSTACK|SA_RESETHAND;
        else
           action.sa_flags = 0;

        if (sigaction(signal_set[i], &action, &old) == -1)
           Log(LOG_ERROR, "sigaction: signum:%d [%d] %s", i, errno, strerror(errno));
     }
     i++;
   } while (i < (sizeof(signal_set)/sizeof(signal_set[0])));
}

int daemon_restart(void) {
   (void)execv(g_argv[0], g_argv);
   Log(LOG_ERROR, "Couldn't restart server: (%d) %s", errno, strerror(errno));
   exit(-1);
   return -1;
}

void posix_signal_quiet(void) {
   sigset_t sigset;
   int signals[] = { SIGABRT, SIGQUIT, SIGKILL, SIGTERM, SIGSYS };
   int sigign[] = { SIGCHLD, SIGPIPE, SIGHUP, SIGSTOP, SIGTSTP, SIGCONT };
   /* these should be by debugger: SIGTRAP, SIGFPE, SIGSEGV */
   sigemptyset(&sigset);

   for (int i = 0; i <= sizeof(signals) / sizeof(signals[0]); i++)
       sigaddset(&sigset, signals[i]);

   pthread_sigmask(SIG_SETMASK, &sigset, NULL);

   for (int i = 0; i <= sizeof(sigign) / sizeof(sigign[0]); i++)
       signal(sigign[i], SIG_IGN);
}
