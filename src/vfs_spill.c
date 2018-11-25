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
 * FUSE VFS Spillover file support
 *
 * This file should NEVER be included directly outside
 * of vfs.c, in fact we throw an error if this
 * is done
 */
#include <signal.h>
#include <errno.h>
#include "conf.h"
#include "database.h"
#include "evt.h"
#include "logger.h"
#include "memory.h"
#include "pkg.h"
#include "vfs.h"
#include "timestr.h"

/* Write-type operations which should return EROFS */
void vfs_spill_setattr(fuse_req_t req, fuse_ino_t ino,
                             struct stat *attr, int to_set, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_mknod(fuse_req_t req, fuse_ino_t ino,
                           const char *name, mode_t mode, dev_t rdev) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_mkdir(fuse_req_t req, fuse_ino_t ino, const char *name, mode_t mode) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_symlink(fuse_req_t req, const char *link, fuse_ino_t parent, const char *name) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_unlink(fuse_req_t req, fuse_ino_t ino, const char *name) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_rmdir(fuse_req_t req, fuse_ino_t ino, const char *namee) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                            fuse_ino_t newparent, const char *newname) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char *newname) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                           size_t size, off_t off, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}

void vfs_spill_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                              const char *value, size_t size, int flags) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_spill_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name) {
   /*
    * XXX: which is proper: ENOSUP, EACCES, ENOATTR? 
    */
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, ENOTSUP);
}

void vfs_spill_create(fuse_req_t req, fuse_ino_t ino, const char *name,
                            mode_t mode, struct fuse_file_info *fi) {
   Log(LOG_DEBUG, "[%lu] %s:%d:%s", time(NULL), __FILE__, __LINE__, __FUNCTION__);
   fuse_reply_err(req, EROFS);
}
