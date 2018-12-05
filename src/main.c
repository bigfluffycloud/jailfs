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
#include "debugger.h"
struct conf conf;

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
   struct fuse_args margs = FUSE_ARGS_INIT(0, NULL);
   conf.born = conf.now = time(NULL);
   umask(0);

   host_init();
   atexit(goodbye);
   signal_init();
   evt_init();
   blockheap_init();

   // Always require jail top-level dir (where jailfs.cf lives)
   if (argc > 1) {
      chdir(argv[1]);
   } else
      usage(argc, argv);

   conf.dict = dconf_load("jailfs.cf");

   // open log file, if its valid, otherwise assume debug mode and use stdout 
   if ((conf.log_fp = log_open(dconf_get_str("path.log", "jailfs.log"))) == NULL)
      conf.log_fp = stdout;

   dlink_init();
   pkg_init();
   inode_init();

   if (dconf_get_bool("sys.daemonize", 0) == 1) {
      fprintf(stdout, "going bg, bye!\n");
      /*
       * XXX: add daemon crud 
       */
   }

   // Create our PID file...
   if (pidfile_open(dconf_get_str("path.pid", "borked.pid"))) {
      Log(LOG_FATAL, "Failed opening PID file. Are we already running?");
      return 1;
   }

   Log(LOG_INFO, "jailfs: Package filesystem %s starting up...", VERSION);
   Log(LOG_INFO, "Copyright (C) 2012-2018 bigfluffy.cloud -- See LICENSE in distribution package for terms of use");

   if (conf.log_level == LOG_DEBUG) {
      Log(LOG_WARNING, "Log level is set to DEBUG. Please use info or lower in production");
      Log(LOG_WARNING, "You can always toggle individual components (debug.*) to adjust the output");
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

   /// XXX: ToDo - Spawn various threads here before entering the main loop

   debug_symtab_lookup("Log", NULL);

   // Main loop
   while (!conf.dying) {
      // XXX: We will communicate with worker threads ONLY from main loop
      ev_loop(evt_loop, 0);
   }

   Log(LOG_INFO, "shutting down...");
   goodbye();
   return EXIT_SUCCESS;
}
