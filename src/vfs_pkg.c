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
/*
 * FUSE VFS Package file operations
 *
 * This file should NEVER be included directly outside
 * of vfs_fuse.c, in fact we throw an error if this
 * is done
 */
#include <signal.h>
#include <errno.h>
#include "conf.h"
#include "db.h"
#include "evt.h"
#include "logger.h"
#include "memory.h"
#include "pkg.h"
#include "vfs.h"
#include "timestr.h"

void vfs_fuse_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   pkg_inode_t *i;
   struct stat sb;
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);

   if ((i = db_query(QUERY_INODE, "SELECT * FROM files WHERE inode = %d", (u_int32_t) ino)) != NULL) {
      /*
       * XXX: do stuff ;) 
       */
      sb.st_ino = ino;
      sb.st_mode = i->st_mode;
      sb.st_size = i->st_size;
      sb.st_uid = i->st_uid;
      sb.st_gid = i->st_gid;
      blockheap_free(inode_heap, i);
   }

   /*
    * Did we get a valid response? 
    */
   if (sb.st_ino) {
      fuse_reply_attr(req, &sb, 0.0);
      Log(LOG_DEBUG, "got attr.st_ino: %d", sb.st_ino);
   } else {
      Log(LOG_DEBUG, "couldn't find attr.st_ino");
      fuse_reply_err(req, ENOENT);
   }
}

void vfs_fuse_access(fuse_req_t req, fuse_ino_t ino, int mask) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, 0);             /* success */
}

void vfs_fuse_readlink(fuse_req_t req, fuse_ino_t ino) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOSYS);
}

void vfs_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOENT);
}

void vfs_fuse_readdir(fuse_req_t req, fuse_ino_t ino,
                             size_t size, off_t off, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOENT);
}

void vfs_fuse_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOENT);
}

void vfs_fuse_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   struct vfs_handle *fh;
   fh = blockheap_alloc(vfs_handle_heap);
   fi->fh = ((uint64_t) fh);
   pkg_inode_t *i;
   struct stat sb;
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);

   /* Check if a spillover file exists */
   if ((i = db_query(QUERY_INODE, "SELECT * FROM spillover WHERE inode = %d", (u_int32_t) ino))) {
   /* If not, maybe this exists in a file? */
   } else if ((i = db_query(QUERY_INODE, "SELECT * FROM files WHERE inode = %d", (u_int32_t) ino))) {
      /*
       * XXX: do stuff ;) 
       */
   }

   // Nope, it doesn't exist
   if (!i) {
      // If process didn't use O_CREAT, return not found
      if (!(fi->flags & O_CREAT)) {
         fuse_reply_err(req, ENOENT);
      } else {
         // XXX: We should create a new fill in spillover
         // and return it's inode. For now we return read-only
         fuse_reply_err(req, EROFS);
      }
   }

   blockheap_free(inode_heap, i);
   fuse_reply_open(req, fi);
}

void vfs_fuse_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, 0);             /* success */
}

void vfs_fuse_read(fuse_req_t req, fuse_ino_t ino,
                          size_t size, off_t off, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
/*      reply_buf_limited(req, hello_str, strlen(hello_str), off, size); */
   fuse_reply_err(req, EBADF);
}

void vfs_fuse_statfs(fuse_req_t req, fuse_ino_t ino) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOSYS);
}

void vfs_fuse_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_fuse_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size) {
   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_fuse_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
   struct fuse_entry_param e;

   Log(LOG_DEBUG, "%s:%d:%s", __FILE__, __LINE__, __FUNCTION__);
#if	0
   if (parent != 1)                    /* XXX: need checks here */
#endif
      fuse_reply_err(req, ENOENT);
#if	0
   else {
      memset(&e, 0, sizeof(e));
      e.ino = 2;                       /* Inode number */
      e.attr_timeout = 1.0;
      e.entry_timeout = 1.0;
      /*
       * XXX: stat 
       */
      vfs_fuse_stat(e.ino, &e.attr);
      fuse_reply_entry(req, &e);
   }
#endif
}
