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
#if	!defined(__PKG_DB_H)
#define	__PKG_DB_H

/* 'import' a package, called by vfs watcher */
extern int  pkg_import(const char *path);

/* 'forget' a package, called by vfs_watcher */
extern int  pkg_forget(const char *path);

#endif	// !defined(__PKG_DB_H)
