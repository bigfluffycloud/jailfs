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
#if	!defined(inode_h)
#define	inode_h

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

extern void vfs_inode_init(void);
extern void vfs_inode_fini(void);

#endif                                 /* !defined(inode_h) */
