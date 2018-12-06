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
#include <sys/types.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdarg.h>
#include "balloc.h"
#include "conf.h"
#include "logger.h"
#include "memory.h"
#include "pkg.h"
#include "pkg_db.h"
#include "util.h"
#define	__VFS_C
#include "vfs.h"

u_int32_t   vfs_root_inode;

// Block allocator heaps
BlockHeap  *vfs_handle_heap = NULL;
BlockHeap  *vfs_watch_heap = NULL;

// Doubly-linked list
dlink_list  vfs_watch_list;

/* We do not want to infinitely recurse... */
#define	MAX_RECURSE	16

static void vfs_dir_walk_recurse(const char *path, int depth) {
   DIR        *d;
   struct dirent *r;
   char       *ext;
   char        buf[PATH_MAX];

   if ((d = opendir(path)) == NULL) {
      Log(LOG_ERROR, "opendir %s failed, skipping", path);
      return;
   }

   while ((r = readdir(d)) != NULL) {
      /*
       * Skip over hidden/disabled items 
       */
      if (r->d_name[0] == '.')
         continue;
      memset(buf, 0, PATH_MAX);
      snprintf(buf, PATH_MAX - 1, "%s/%s", path, r->d_name);

      if (is_dir(buf)) {
         if (++depth < MAX_RECURSE)
            vfs_dir_walk_recurse(buf, depth);
         else
            Log(LOG_INFO, "%s: reached maximum depth (%d) in %s", __FUNCTION__, MAX_RECURSE, path);
      } else {
         ext = strrchr(buf, '.');
         

         if (ext == NULL)
            return;

         pkg_import(buf);
      }
   }

   closedir(d);
}
#undef MAX_RECURSE

int vfs_dir_walk(void) {
   char        buf[PATH_MAX];
   char       *p;

   memcpy(buf, dconf_get_str("path.pkg", "/pkg"), PATH_MAX);

   /*
    * recurse each part of the optionally ':' seperated list 
    */
   for (p = strtok(buf, ":\n"); p; p = strtok(NULL, ":\n")) {
      /*
       * Run the recursive walker 
       */
      vfs_dir_walk_recurse(p, 1);
   }
   return EXIT_SUCCESS;
}

/* Be sure to free these :O */
struct vfs_lookup_reply {
   u_int32_t	seq;
   u_int32_t	status;
   int		pkgid;		// Package ID if appropriate
   int		fileid;		// File ID from database
};
typedef struct vfs_lookup_reply vfs_lookup_reply;

vfs_lookup_reply *vfs_resolve_path(const char *path) {
    vfs_lookup_reply *res = mem_alloc(sizeof(vfs_lookup_reply));

    // XXX: Locate the file and return vfs_file handle for it
    // - Look in jail/config/
    // - Look in jail/state/*.spill
    // - Look in jail/pkg/
    // - Look in ../pkg/
    // If not found, create in spillover (if O_CREAT) or return ENOENT.
    if (dconf_get_bool("debug.vfs", 0) == 1) {
       Log(LOG_DEBUG, "vfs:resolve_path(%s): seq<%d> status<%d> pkg<%d> file<%d>",
          res->seq, res->status, res->pkgid, res->fileid);
    }
    return res;
}
