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
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "balloc.h"

#if	!defined(__VFS_H)
#define	__VFS_H

struct vfs_handle {
//   char        pkg_file[PATH_MAX];     /* package path */
//   off_t       pkg_offset;             /* offset within package */
   int	       pkgid;	      /* key used to find the mmap pointer */
   u_int32_t   fileid;        /* file ID inside the package XXX: should this be inode #?? */
   char        file[PATH_MAX];         /* file name */
   size_t      len;                    /* length of file */
//   off_t       offset;                 /* current offset in file */
   char       *maddr;                  /* mmap()'d region of package */
};

struct vfs_watch {
   u_int32_t   mask;
   int         fd;
   char        path[PATH_MAX];
};

struct vfs_fake_stat {
   u_int32_t   st_ino;
   mode_t      st_mode;
   off_t       st_size;
   uid_t       st_uid;
   gid_t       st_gid;
   time_t      st_time;
   char        path[PATH_MAX];
};

typedef struct vfs_handle vfs_handle_t;
typedef struct vfs_watch vfs_watch_t;

extern int  vfs_dir_walk(void);

extern BlockHeap *vfs_handle_heap;
extern BlockHeap *vfs_inode_heap;
extern BlockHeap *vfs_watch_heap;

extern u_int32_t vfs_root_inode;
extern dlink_list vfs_watch_list;

extern void *thread_vfs_init(void *data);
extern void *thread_vfs_fini(void *data);

#include "vfs_fuse.h"
#include "vfs_inode.h"
#include "vfs_inotify.h"
#include "vfs_spill.h"
#include "vfs_mimetype.h"

#endif	// !defined(__VFS_H)
