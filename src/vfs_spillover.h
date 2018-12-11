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
#if	!defined(__VFS_SPILLOVER_H)
#define	__VFS_SPILLOVER_H

extern void vfs_spill_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi);
extern void vfs_spill_mknod(fuse_req_t req, fuse_ino_t ino, const char *name, mode_t mode, dev_t rdev);
extern void vfs_spill_mkdir(fuse_req_t req, fuse_ino_t ino, const char *name, mode_t mode);
extern void vfs_spill_symlink(fuse_req_t req, const char *link, fuse_ino_t parent, const char *name);
extern void vfs_spill_unlink(fuse_req_t req, fuse_ino_t ino, const char *name);
extern void vfs_spill_rmdir(fuse_req_t req, fuse_ino_t ino, const char *namee);
extern void vfs_spill_rename(fuse_req_t req, fuse_ino_t parent, const char *name, fuse_ino_t newparent, const char *newname);
extern void vfs_spill_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char *newname);
extern void vfs_spill_write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi);
extern void vfs_spill_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name, const char *value, size_t size, int flags);
extern void vfs_spill_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name);
extern void vfs_spill_create(fuse_req_t req, fuse_ino_t ino, const char *name, mode_t mode, struct fuse_file_info *fi);

#endif	// !defined(__VFS_SPILLOVER_H)
