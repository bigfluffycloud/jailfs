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
#define	FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
extern struct fuse_args vfs_fuse_args;
extern void *fuse_thr(void *unused);
extern void vfs_fuse_fini(void);
extern void vfs_fuse_init(void);
extern void vfs_fuse_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
extern void vfs_fuse_access(fuse_req_t req, fuse_ino_t ino, int mask);
extern void vfs_fuse_readlink(fuse_req_t req, fuse_ino_t ino);
extern void vfs_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
extern void vfs_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);
extern void vfs_fuse_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
extern void vfs_fuse_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
extern void vfs_fuse_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
extern void vfs_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);
extern void vfs_fuse_statfs(fuse_req_t req, fuse_ino_t ino);
extern void vfs_fuse_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size);
extern void vfs_fuse_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size);
extern void vfs_fuse_lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
