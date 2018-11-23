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
#include <sys/stat.h>
#include <errno.h>
static __inline int is_dir(const char *path) {
   struct stat sb;

   stat(path, &sb);

   if (S_ISDIR(sb.st_mode))
      return 1;

   return 0;
}

static __inline int file_exists(const char *path) {
   struct stat sb;
   stat(path, &sb);

   if (errno == ENOENT)
      return 0;

   return 1;
}
