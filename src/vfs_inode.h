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
#if	!defined(inode_h)
#define	inode_h

#include "balloc.h"
struct pkg_inode {
   u_int32_t   st_ino;
   mode_t      st_mode;
   off_t       st_size;
   off_t       st_off;
   uid_t       st_uid;
   gid_t       st_gid;
   time_t      st_time;
};

typedef struct pkg_inode pkg_inode_t;

BlockHeap  *inode_heap;

extern void inode_init(void);
extern void inode_fini(void);

#endif                                 /* !defined(inode_h) */
