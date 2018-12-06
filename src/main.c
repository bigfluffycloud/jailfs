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

struct conf conf;
ThreadPool *threads_main;

struct ThreadCreator {
   char *name;
   void (*init)(void *);
   void (*fini)(void *);
} threads[] = {
   {
      .name = "db",
//      .init = &thread_db_init,
//      .fini = &thread_db_fini,
   },
   {
      .name = "shell",
      .init = &thread_shell_init,
//      .fini = &thread_shell_fini,
   },
   {
      .name = "vfs",
//      .init = &thread_vfs_init,
//      .fini = &thread_vfs_fini,
   },
   {
      .name = NULL
   }
};

// Called on exit to cleanup..
void goodbye(void) {
   char *pidfile = dconf_get_str("path.pid", NULL);
   dconf_fini();
   vfs_watch_fini();
   vfs_fuse_fini();
   umount(conf.mountpoint);
   inode_fini();
   dlink_fini();
   log_close(conf.log_fp);

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
   struct fuse_args margs = FUSE_ARGS_INIT(0, NULL);

   conf.born = conf.now = time(NULL);
   umask(0077);

   // Lock the Big Lock until everything is running
   pthread_mutex_lock(&core_ready_m);
   host_init();
   atexit(goodbye);
   // Block all non-fatal signals until we are up and running
   posix_signal_quiet();
   evt_init();
   blockheap_init();

   // Always require jail top-level dir (where jailfs.cf lives)
   if (argc > 1) {
      chdir(argv[1]);
   } else
      usage(argc, argv);

   conf.dict = dconf_load("jailfs.cf");
   i18n_init();

   // open log file, if its valid, otherwise assume debug mode and use stdout 
   if ((conf.log_fp = log_open(dconf_get_str("path.log", "jailfs.log"))) == NULL)
      conf.log_fp = stdout;

   dlink_init();
   pkg_init();
   inode_init();

   Log(LOG_INFO, "jailfs: Package filesystem %s starting up...", VERSION);
   Log(LOG_INFO, "Copyright (C) 2012-2018 bigfluffy.cloud -- See LICENSE in distribution package for terms of use");

   // Create our PID file...
   if (pidfile_open(dconf_get_str("path.pid", "borked.pid"))) {
      Log(LOG_FATAL, "Failed opening PID file. Are we already running?");
      return 1;
   }

   if (conf.log_level == LOG_DEBUG) {
      Log(LOG_WARNING, "Log level is set to DEBUG. Please use info or lower in production");
      Log(LOG_WARNING, "You can disable uninteresting debug sources by setting config:[general]/debug.* to false");
   }

   // Start cron
   if (conf.log_level == LOG_DEBUG)
      Log(LOG_DEBUG, "starting periodic task scheduler (cron)");

   cron_init();

   // Figure out where we're supposed to build this jail
   if (!conf.mountpoint)
      conf.mountpoint = dconf_get_str("path.mountpoint", "chroot/");

   // XXX: TODO: We need to unlink mountpoint/.keepme if it exist before calling fuse...

   // only way to make gcc happy...argh;) -bk 
   vfs_fuse_args = margs;

   /*
    * The fuse_mount() options get modified, so we always rebuild it 
    */
   if ((fuse_opt_add_arg(&vfs_fuse_args, argv[0]) == -1 ||
        fuse_opt_add_arg(&vfs_fuse_args, "-o") == -1 ||
        fuse_opt_add_arg(&vfs_fuse_args, "nonempty,allow_other") == -1))
      Log(LOG_ERROR, "Failed to set FUSE options.");

   umount(conf.mountpoint);
   vfs_fuse_init();
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
            Log(LOG_ERROR, "mounting tmpfs on cache-dir %s failed: %d (%s)", cache, errno, strerror(errno));
            exit(1);
         }
      }
   }

   Log(LOG_INFO, "Starting threads...");

   threads_main = threadpool_init("main", NULL);
   // Launch main threads
   thr_cnt = sizeof(threads)/sizeof(struct ThreadCreator) - 1;
   i = 0;
   Log(LOG_INFO, "->");
   do {
      if (threads[i].name != NULL) {
         if (!threads[i].init) {
            Log(LOG_ERROR, "module entry %s has no init function", threads[i].name);
            i++;
            continue;
         }

         if (thread_create(threads_main, threads[i].init, threads[i].fini, NULL, threads[i].name) == NULL) {
            Log(LOG_ERROR, "failed starting thread main:%s", threads[i].name);
            abort();
         }
         Log(LOG_INFO, "started thread main:%s", threads[i].name);
      }
      i++;
   } while (i <= thr_cnt);

   Log(LOG_INFO, "====");
   list_iter_p m_cur = list_iterator(Modules, FRONT);
   Module *mod;
   do {
     Log(LOG_INFO, "Module @ %x", mod);
   } while ((mod = list_next(m_cur)));

   Log(LOG_INFO, "Ready to accept requests.");


   signal_init();

   core_ready = 1;
   pthread_mutex_unlock(&core_ready_m);
   pthread_cond_broadcast(&core_ready_c);

   /// XXX: ToDo - Spawn various threads here before entering the main loop
   debug_symtab_lookup("Log", NULL);

   // Looks like everything came up OK, detach if configured to do so...
   if (dconf_get_bool("sys.daemonize", 0) == 1)
      host_detach();

   while (!conf.dying) {
      // Handle profiling events
      if (profiling_newmsg) {
         Log(LOG_DEBUG, "profiling: %s", profiling_msg);
         profiling_newmsg = 0;
      }
      // XXX: We will communicate with worker threads ONLY from main loop
      ev_loop(evt_loop, 0);
   }

   umount(cache);
   Log(LOG_INFO, "shutting down...");
   goodbye();
   return EXIT_SUCCESS;
}
