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
 */
#define	VERSION "0.0.2"
#include <sys/mount.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "conf.h"
#include "database.h"
#include "cron.h"
#include "logger.h"
#include "pkg.h"
#include "dlink.h"
#include "signals.h"
#include "threads.h"
#include "vfs.h"
#include "mimetypes.h"
#include "unix.h"
#include "cron.h"
#include "util.h"
#include "shell.h"
#include "debugger.h"
#include "module.h"
#include "profiling.h"

ThreadPool *threads_main;

// These threads need to come up in a specific order, unlike modules...
struct ThreadCreator {
   char *name;
   void (*init)(void *);
   void (*fini)(void *);
} threads[] = {
   {
      .name = "logger",
      .init = &thread_logger_init,
      .fini = &thread_logger_fini,
   },
   {
      .name = "db",
//      .init = &thread_db_init,
//      .fini = &thread_db_fini,
   },
   {
      .name = "vfs",
      .init = &thread_vfs_init,
      .fini = &thread_vfs_fini,
   },
   {
      .name = "shell",
      .init = &thread_shell_init,
      .fini = &thread_shell_fini,
   },
   {
      .name = NULL
   }
};

// Called on exit to cleanup..
void goodbye(void) {
   char *pidfile = dconf_get_str("path.pid", NULL);
   char *mp = NULL;
   Log(LOG_INFO, "shutting down...");
   conf.dying = true;

   // Unmount the mountpoint
   if ((mp = dconf_get_str("path.mountpoint", NULL)) != NULL)
      umount(mp);

   // Unmount the cache (if mounted)
   if (strcasecmp("tmpfs", dconf_get_str("cache.type", NULL)) == 0) {
      if ((mp = dconf_get_str("path.cache", NULL)) != NULL)
         umount(mp);
   }

   vfs_watch_fini();
   vfs_fuse_fini();
   inode_fini();
   dlink_fini();
   dconf_fini();

   if (pidfile && file_exists(pidfile))
      unlink(pidfile);

   exit(EXIT_SUCCESS);
}

void usage(int argc, char **argv) {
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
   char *cache = NULL;

   // XXX: Parses commandline arguments (should be minimal)
   //
   // Always require jail top-level dir (where jailfs.cf lives)
   if (argc > 1) {
      chdir(argv[1]);
   } else
      usage(argc, argv);

   // Begin Fuckery!
   pthread_mutex_lock(&core_ready_m);		// Set the Big Lock (TM)

   ///////
   // What do we do here?
   //////

   host_init();					// Platform initialization
   conf.born = conf.now = time(NULL);		// Set birthday and clock (cron maintains)
   umask(0077);					// Restrict umask on new files
   evt_init();					// Socket event handler
   blockheap_init();				// Block heap allocator
   conf.dict = dconf_load("jailfs.cf");		// Load config
   cron_init();					// Periodic events
   i18n_init();					// Load translations
   signal_init();				// Setup POSIX siganls
   dlink_init();				// Doubly linked lists
   pkg_init();					// Package utilities
   inode_init();				// Initialize inode crud
   Log(LOG_INFO, "jailfs: container filesystem %s starting up...", VERSION);
   Log(LOG_INFO, "Copyright (C) 2012-2018 bigfluffy.cloud -- See LICENSE in distribution package for terms of use");

   if (pidfile_open(dconf_get_str("path.pid", NULL))) {
      Log(LOG_EMERG, "Failed opening PID file. Are we already running?");
      return 1;
   }

   if (strcasecmp(dconf_get_str("log.level", "debug"), "debug") == 0) {
      Log(LOG_WARNING, "Log level is set to DEBUG. Please use info or lower in production");
      Log(LOG_WARNING, "You can disable uninteresting debug sources by setting config:[general]/debug.* to false");
   }
   mimetype_init();

   Log(LOG_INFO, "Opening database %s", dconf_get_str("path.db", ":memory"));
   db_open(dconf_get_str("path.db", ":memory"));

   // Add inotify watchers for paths in %{path.pkg}
   if (dconf_get_bool("pkgdir.inotify", 0) == 1)
      vfs_watch_init();

   // Load all packages in %{path.pkg}} if enabled
   if (dconf_get_bool("pkgdir.prescan", 0) == 1)
      vfs_dir_walk();

   Log(LOG_INFO, "jail at %s/%s is now ready!", get_current_dir_name(), conf.mountpoint);
  
   // Caching support
   cache = dconf_get_str("path.cache", NULL);
   if (strcasecmp("tmpfs", dconf_get_str("cache.type", NULL)) == 0) {
      if (cache != NULL) {
         int rv = -1;

         // If .keepme exists in cachedir (from git), remove it
         char tmppath[PATH_MAX];
         memset(tmppath, 0, sizeof(tmppath));
         snprintf(tmppath, sizeof(tmppath) - 1, "%s/.keepme", cache);
         if (file_exists(tmppath))
            unlink(tmppath);

         if ((rv = mount("jailfs-cache", cache, "tmpfs", 0, NULL)) != 0) {
            Log(LOG_ERR, "mounting tmpfs on cache-dir %s failed: %d (%s)", cache, errno, strerror(errno));
            exit(1);
         }
      }
   }

   Log(LOG_INFO, "Starting threads...");
   threads_main = threadpool_init("main", NULL);
   thr_cnt = sizeof(threads)/sizeof(struct ThreadCreator) - 1;
   i = 0;
   do {
      if (threads[i].name != NULL) {
         if (!threads[i].init) {
            Log(LOG_ERR, "module entry %s has no init function", threads[i].name);
            i++;
            continue;
         }

         if (thread_create(threads_main, threads[i].init, threads[i].fini, NULL, threads[i].name) == NULL) {
            Log(LOG_ERR, "failed starting thread main:%s", threads[i].name);
            abort();
         }
         Log(LOG_INFO, "started thread main:%s", threads[i].name);
      }
      i++;
   } while (i <= thr_cnt);

   list_iter_p m_cur = list_iterator(Modules, FRONT);
   Module *mod;
   do {
     if (mod == NULL)
        continue;

     Log(LOG_INFO, "Module @ %x", mod);
   } while ((mod = list_next(m_cur)));


   // Test symbol lookup (needed for debugger) and generate log error if cannot find symtab...
   debug_symtab_lookup("Log", NULL);

   // We are in flight, allow children to begin doing stuff & things!
   core_ready = 1;
   pthread_mutex_unlock(&core_ready_m);
   pthread_cond_broadcast(&core_ready_c);
   Log(LOG_INFO, "Ready to accept requests.");

   // Main loop for libev
   while (!conf.dying) {
      if (profiling_newmsg) {	// profiling events
         Log(LOG_DEBUG, "profiling: %s", profiling_msg);
         profiling_newmsg = 0;
      }
      ev_loop(evt_loop, 0);
   }

   goodbye();
   return EXIT_SUCCESS;
}
