/*
 * tk/servers/jailfs:
 *	Package filesystem. Allows mounting various package files as
 *   a read-only (optionally with spillover file) overlay filesystem
 *   via FUSE or (eventually) LD_PRELOAD library.
 *
 * Copyright (C) 2012-2019 BigFluffy.Cloud <joseph@bigfluffy.cloud>
 *
 * Distributed under a MIT license. Send bugs/patches by email or
 * on github - https://github.com/bigfluffycloud/jailfs/
 *
 * No warranty of any kind. Good luck!
 */
#include <lsd/lsd.h>
// This is sloppy.. We need to clean up the headers...
#include "conf.h"
#include "unix.h"
#include "cron.h"
#include "threads.h"
#include "cron.h"
#include "shell.h"
#include "debugger.h"
#include "module.h"
#include "i18n.h"
#include "cell.h"
#include "gc.h"
#include "vfs.h"
#include "database.h"
BlockHeap  *main_heap;
ThreadPool *main_threadpool;

/// This nonsense needs moved to an API call (module_register() or likes)
struct ThreadCreator {
   char *name;
   void *(*init)(void *);
   void *(*fini)(void *);
   int isolated;
} main_threads[] = {
  // The order here is sorta significant - logger must be first and shell last!
  { "db", thread_db_init, thread_db_fini, 0 },
  { "vfs", thread_vfs_init, thread_vfs_fini, 0 },
  { "cell", thread_cell_init, thread_cell_fini, 1 },
  { "shell", thread_shell_init, thread_shell_fini, 1 },
  { NULL, NULL, NULL }
};

// Called on exit to cleanup..
void goodbye(void) {
   char *pidfile = dconf_get_str("path.pid", NULL);
   char *mp = NULL;
   Log(LOG_INFO, "shutting down...");
   conf.dying = true;

   dlink_fini();
   dconf_fini();

   if (pidfile && file_exists(pidfile))
      unlink(pidfile);

   exit(EXIT_SUCCESS);
}

static void usage(int argc, char **argv) {
   printf("Usage: %s <jaildir> [action]\n", basename(argv[0]));
   printf("Compose a chroot jail based on a shared package pool.\n\n");
   printf("Options:\n");
   printf("\t<jaildir>\t\tThe directory containing jailfs.cf, etc for the desired jail\n");
   printf("\t[action]\t\tOptionally an action to take on the jail ([start]|stop|status)\n\n");
   exit(1);
}

int main(int argc, char **argv) {
   int         fd;
   int i, thr_cnt = 0;

   // XXX: Parses commandline arguments (should be minimal)

   Log(LOG_INFO, "jailfs: container filesystem %s starting up...", PKG_VERSION);
   Log(LOG_INFO, "Copyright (C) 2012-2019 bigfluffy.cloud -- See LICENSE in distribution package for terms of use");

   if (argc > 1) {
      if (chdir(argv[1])) {
         // Ignore
      }
   } else
      usage(argc, argv);

   // XXX: we need to clone(), etc to create a fresh namespace

   // Begin Fuckery!
   pthread_mutex_lock(&core_ready_m);		// Set the Big Lock (TM)
   host_init();					// Platform initialization
   conf.born = conf.now = time(NULL);		// Set birthday and clock (cron maintains)
   umask(0077);					// Restrict umask on new files
   evt_init();					// Socket event handler
   blockheap_init();				// Block heap allocator
   // Start garbage collector
   evt_timer_add_periodic(gc_all,
     "gc.blockheap",
      timestr_to_time(dconf_get_str("tuning.timer.blockheap_gc", NULL), 60));
   api_init();				// Initialize MASTER thread
   conf.dict = dconf_load("jailfs.cf");		// Load config
   log_open(dconf_get_str("path.log", "file://jailfs.log"));
   cron_init();					// Periodic events
   i18n_init();					// Load translations
   dlink_init();				// Doubly linked lists
   pkg_init();					// Package utilities

   if (pidfile_open(dconf_get_str("path.pid", NULL))) {
      Log(LOG_EMERG, "Failed opening PID file. Are we already running?");
      return 1;
   }

   if (strcasecmp(dconf_get_str("log.level", "debug"), "debug") == 0) {
      Log(LOG_WARNING, "Log level is set to DEBUG. Please use info or lower in production");
      Log(LOG_WARNING, "You can disable uninteresting debug sources by setting config:[general]/debug.* to false");
   }

#if	defined(CONFIG_MODULES)
   // Initialize configured modules.
   Log(LOG_INFO, "Loading plugins...");
   list_iter_p m_cur = list_iterator(Modules, FRONT);
   Module *mod;
   do {
     if (mod == NULL)
        continue;

     Log(LOG_INFO, "Plugin @ 0x%x", mod);
   } while ((mod = list_next(m_cur)));
#endif	// defined(CONFIG_MODULES)

   // These are defined at the top of src/main.c
   Log(LOG_INFO, "Starting core threads...");
   main_threadpool = threadpool_init("main", NULL);
   thr_cnt = sizeof(main_threads)/sizeof(struct ThreadCreator) - 1;
   i = 0;
   do {
      if (main_threads[i].name != NULL) {
         if (!main_threads[i].init) {
            i++;
            continue;
         }

         if (thread_create(main_threadpool, main_threads[i].init, main_threads[i].fini, NULL, main_threads[i].name) == NULL) {
            Log(LOG_ERR, "failed starting thread main:%s", main_threads[i].name);
            abort();
         }
         Log(LOG_INFO, "* started thread main:%s", main_threads[i].name);
      }
      i++;
   } while (i <= thr_cnt);

#if	defined(CONFIG_DEBUGGER)
   // Test symbol lookup (needed for debugger) and generate log error if cannot find symtab...
   debug_symtab_lookup("Log", NULL);
#endif

   // We are in flight, allow children to begin doing stuff & things!
   core_ready = 1;
   pthread_mutex_unlock(&core_ready_m);
   pthread_cond_broadcast(&core_ready_c);

   // Main loop for libev
   while (!conf.dying) {
#if	defined(CONFIG_PROFILING)
      if (profiling_newmsg) {	// profiling events
         Log(LOG_DEBUG, "profiling: %s", profiling_msg);
         profiling_newmsg = 0;
      }
#endif	// defined(CONFIG_PROFILING)
      ev_loop(evt_loop, 0);
   }

   goodbye();
   return EXIT_SUCCESS;
}
