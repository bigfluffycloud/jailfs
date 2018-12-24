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
#if	!defined(__DB_H)
#define	__DB_H
#include <sys/types.h>

enum db_query_res_type {
   QUERY_NULL = 0,                     /* no result */
   QUERY_INT,                          /* integer result */
   QUERY_CHAR,                         /* char result */
   QUERY_INODE,                        /* pkgfs_inode result */
};

#if	0	// Future stuff maybe?
struct db_connector {
   /*
    * internal properties 
    */
   void       *hndl;                   /* database handle */
   pthread_mutex_t mutex;              /* mutex */
   u_int16_t   error;                  /* error return */

   /*
    * semi-private methods 
    */
   void        (*begin) (void);
   void        (*commit) (void);
   void        (*rollback) (void);

   /*
    * public methods 
    */
   void       *(*db_query) (enum db_query_res_type type, const char *fmt, ...);
   int         (*db_open) (const char *path);
   void        (*db_destroy) (void);
   void	       (*query)(enum db_query_res_type type, const char *fmt, ...);
};
#endif	// Future stuff?

extern void *db_query(enum db_query_res_type type, const char *fmt, ...);
extern int  db_pkg_add(const char *path);
extern int  db_file_add(int pkg, const char *path, const char type,
                        uid_t uid, gid_t gid, const char *owner, const char *group,
                        size_t size, off_t offset, time_t ctime, mode_t mode, const char *perm);
extern int  db_pkg_remove(const char *path);
extern int  db_file_remove(int pkg, const char *path);

extern void db_begin(void);
extern void db_commit(void);
extern void db_rollback(void);

extern void *thread_db_init(void *data);
extern void *thread_db_fini(void *data);

#endif	// !defined(__DB_H)
