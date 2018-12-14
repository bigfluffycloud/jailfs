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
 *
 * src/pkg_db.c:
 *	Functions to manage the packae database
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <archive.h>
#include <archive_entry.h>
#include "atomicio.h"
#include "balloc.h"
#include "conf.h"
#include "database.h"
#include "cron.h"
#include "logger.h"
#include "memory.h"
#include "pkg.h"
#include "pkg_db.h"
#include "str.h"
#include "timestr.h"

//
// pkg_import: Scan the contents of a package and add them to the VFS view
//
// XXX: We should add a 'preload' option to jailconf to cache all files in
// XXX: important packages. - jm 12/13/18
int pkg_import(const char *path) {
   char       *tmp = NULL;
   int         infd, outfd;
   int         db_id = -1;
   struct archive *a;
   struct archive_entry *aentry;
   int r;
   int pkgid = -1;
   char _f_type = '-';

   /*
    * Upgrading a package should consist of replace it with
    * one with a higher version number than the old one.
    *
    * A flag will be set on the package, indicating it is to be removed
    * and when the refcnt hits ZERO, it will be unlinked by the system.
    * This won't work until the persistent database is in place
    * and will also require the assistance of the package manager to ensure
    * nothing gets left behind. -- For now, it's not gonna happen
    */
   if (pkg_handle_byname(path)) {
      Log(LOG_CRIT, "BUG: Active package %s was changed, ignoring...");
      return EXIT_SUCCESS;
   }

   if (dconf_get_bool("debug.pkg", 0) == 1)
      Log(LOG_DEBUG, "BEGIN import pkg %s", basename(path));

   // Start transaction
   db_begin();

   // Initialize our libarchive reader
   a = archive_read_new();
   archive_read_support_filter_all(a);
   archive_read_support_format_all(a);
   r = archive_read_open_filename(a, path, 0);

   if (r != ARCHIVE_OK) {
      Log(LOG_ERR, "package %s is not valid: libarchive returned %d", path, r);
      return -1;
   }

   // Add the package to the database & get the pkgid for it
   pkgid = db_pkg_add(path);
   Log(LOG_DEBUG, "package %s appears valid, assigning pkgid %d", path, pkgid);

   // Add the achive's file entries to the database...
   while (TRUE) {
      r = archive_read_next_header(a, &aentry);
      if (r == ARCHIVE_EOF)
         break;

      if (r != ARCHIVE_OK) {
         db_rollback();
         Log(LOG_DEBUG, "pkg_import: libarchive read_next_header error %d: %s", r, archive_error_string(a));
         break;
      }

      const char *_f_name = archive_entry_pathname(aentry);
      const char *_f_owner = archive_entry_uname(aentry);
      const char *_f_group = archive_entry_gname(aentry);
      const uid_t _f_uid = archive_entry_uid(aentry);
      const gid_t _f_gid = archive_entry_gid(aentry);
      const mode_t _f_mode = archive_entry_perm(aentry);
      const char *_f_perm = archive_entry_strmode(aentry);
      const struct stat *st = archive_entry_stat(aentry); 

      // XXX: Determine type more sanely
      if (*_f_perm == 'd')
         _f_type = 'd';
      else if (*_f_perm == 'l')
         _f_type = 'l';
      else
         _f_type = 'f';

      // We do not add directories...
      if (_f_type == 'd') {
         archive_read_data_skip(a);
         continue;
      }

      if (dconf_get_bool("debug.pkg", 0) == 1)
         Log(LOG_DEBUG, "+ %s:%s (user: %d %s) (group: %d %s) mode=%o perms=%s size:%lu@%lu",
             basename(path), _f_name, _f_uid, _f_owner, _f_gid, _f_group, _f_mode, _f_perm, st->st_size, 0);

      if (_f_type == 'l') {
         // XXX: Handle symlinks...
      } else {
         db_file_add(pkgid, _f_name, _f_type, _f_uid, _f_gid,
                         _f_owner, _f_group, st->st_size,
                         0, time(NULL), st->st_mode, _f_perm);
      }
   }

   archive_read_close(a);
   if ((r = archive_read_free(a)) != ARCHIVE_OK)
      Log(LOG_ERR, "possible memory leak! archive_read_free() returned %d", r);

   db_commit();

   if (dconf_get_bool("debug.pkg", 0) == 1)
      Log(LOG_INFO, "SUCCESS import pkg %s", basename(path));

   return EXIT_SUCCESS;
}
