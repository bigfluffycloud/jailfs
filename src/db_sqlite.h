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
#if	!defined(__DB_SQLITE_H)
#define	__DB_SQLITE_H

#include <sys/types.h>
#include "vfs_inode.h"

extern void *db_query(enum db_query_res_type type, const char *fmt, ...);
extern int  db_sqlite_open(const char *path);
extern void db_sqlite_close(void);
extern void db_sqlite_close(void);
extern int  db_pkg_add(const char *path);
extern int  db_file_add(int pkg, const char *path, const char type,
                        uid_t uid, gid_t gid, const char *owner, const char *group,
                        size_t size, off_t offset, time_t ctime, mode_t mode, const char *perm);
extern int  db_pkg_remove(const char *path);
extern int  db_file_remove(int pkg, const char *path);
extern void db_begin(void);
extern void db_commit(void);
extern void db_rollback(void);
#endif	// !defined(__DB_SQLITE_H)