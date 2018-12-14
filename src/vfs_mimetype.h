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
#if	!defined(__mimetypes_h)
#define	__mimetypes_h
#include <magic.h>

extern magic_t mimetype_init(void);
extern void mimetype_fini(magic_t ptr);
extern const char *mimetype_by_path(magic_t ptr, const char *path);
extern const char *mimetype_by_fd(magic_t ptr, int fd);

#endif	// !defined(__mimetypes_h)
