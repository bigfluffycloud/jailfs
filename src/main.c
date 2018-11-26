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
#define	VERSION "0.0.2"
#include <sys/mount.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "conf.h"
#include "database.h"
#include "evt.h"
#include "logger.h"
#include "pkg.h"
#include "dlink.h"
#include "signals.h"
#include "threads.h"
#include "vfs.h"
#include "mimetypes.h"
#include "unix.h"
#include "cron.h"

struct conf conf;

void goodbye(void) {
   Log(LOG_INFO, "shutting down...");
   dconf_fini();
   vfs_watch_fini();
   vfs_fuse_fini();
   umount(conf.mountpoint);
   inode_fini();
   dlink_fini();
   log_close(conf.log_fp);
   exit(EXIT_SUCCESS);
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

   if (argc > 1)
      chdir(argv[1]);

   conf.dict = dconf_load("jailfs.cf");

   dlink_init();
   pkg_init();
   inode_init();

   if (dconf_get_bool("sys.daemonize", 0) == 1) {
      fprintf(stdout, "going bg, bye!\n");
      /*
       * XXX: add daemon crud 
       */
   }

   /*
    * open log file, if its valid, otherwise assume debug mode and use stdout 
    */
   if ((conf.log_fp = log_open(dconf_get_str("path.log", "jailfs.log"))) == NULL)
      conf.log_fp = stdout;

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
   /*
    * only way to make gcc happy...argh;) -bk 
    */
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

   // Add inotify watchers for paths in %{path.pkgdir}
   vfs_watch_init();

   // Load all packages in %{path.pkgdir}
   vfs_dir_walk();

   Log(LOG_INFO, "jail at %s/%s is now ready!", get_current_dir_name(), conf.mountpoint);

   /// XXX: ToDo - Spawn various threads here before entering the main loop

   // Main loop
   while (!conf.dying) {
      // XXX: We will communicate with worker threads ONLY from main loop
      ev_loop(evt_loop, 0);
   }

   return EXIT_SUCCESS;
}
