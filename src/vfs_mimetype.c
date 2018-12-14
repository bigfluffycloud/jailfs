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

/* Wrapper around libmagic to support mime-type identification */
#include "conf.h"
#include "logger.h"
#include "vfs_mimetype.h"

magic_t mimetype_init(void) {
  magic_t tmp;
  int flags = MAGIC_SYMLINK|MAGIC_MIME|MAGIC_PRESERVE_ATIME;

  if (dconf_get_bool("debug.vfs.mime", 0) == 1)
     flags |= MAGIC_DEBUG;

  tmp = magic_open(flags);
  magic_load(tmp, NULL);
  Log(LOG_INFO, "[vfs] file magic (mime-type) services started succesfully");
  return tmp;
}

void mimetype_fini(magic_t ptr) {
  magic_close(ptr);
}

const char *mimetype_by_path(magic_t ptr, const char *path) {
  return magic_file(ptr, path);
}

const char *mimetype_by_fd(magic_t ptr, int fd) {
  return magic_descriptor(ptr, fd);
}
