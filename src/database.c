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
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sqlite3.h>
#include "balloc.h"
#include "conf.h"
#include "database.h"
#include "vfs.h"
#include "logger.h"
#include "memory.h"
#include "util.h"
#define	SQL_BUFSIZE	8192

static sqlite3 *sqlite_db = NULL;
static pthread_mutex_t db_mutex;

/* transaction primitives */
void db_begin(void) {
   pthread_mutex_lock(&db_mutex);
   db_query(QUERY_NULL, "BEGIN;");
}

void db_commit(void) {
   db_query(QUERY_NULL, "COMMIT;");
   pthread_mutex_unlock(&db_mutex);
}

void db_rollback(void) {
   db_query(QUERY_NULL, "ROLLBACK;");
   pthread_mutex_unlock(&db_mutex);
}

/* Reuse a prepared statement */
static int db_statement_reset(sqlite3_stmt * s) {
   sqlite3_clear_bindings(s);
   return sqlite3_reset(s);
}

/* Create database structure, etc */
static void db_initialize(void) {
   db_begin();
   db_query(QUERY_NULL, "DROP TABLE IF EXISTS packages");
   db_query(QUERY_NULL, "DROP TABLE IF EXISTS files");
   db_query(QUERY_NULL, "DROP TABLE IF EXISTS spillover");
   db_query(QUERY_NULL, "CREATE TABLE packages (path TEXT, version INTEGER, id INTEGER PRIMARY KEY AUTOINCREMENT);");
   /*
    * package: <uint32_t> package which owns this file
    *    path: <char *>   file/link/directory path
    *    type: <char>     valid values: [bcdilps]
    *                     (f)ile, (d)irectory, (l)ink, (p)ipe
    *                     f(i)fo, (c)haracter, (b)lock, (s)ocket
    *     uid: <uid_t>    object owner
    *     gid: <gid_t>    object group
    *   owner: <char *>   owner name
    *     grp: <char *>   group name
    *    size: <size_t>   file size
    *    mode: <mode_t>   file mode
    *  offset: <off_t>    within the package
    *   ctime: <time_t>   creation time
    *   inode: <uint32_t> unique node number for this object
    */
   db_query(QUERY_NULL, "CREATE TABLE files (package INTEGER, path TEXT, type TEXT, uid INTEGER, gid INTEGER, owner VARCHAR(128), grp VARCHAR(128), size INTEGER, mode INTEGER, offset INTEGER, ctime INTEGER, target INTEGER, inode INTEGER PRIMARY KEY AUTOINCREMENT);");
   db_query(QUERY_NULL, "INSERT INTO files (package, path, type, size, offset, owner, grp, mode) VALUES ('', '/', 'd', 0, 0, 0, 0, 16895);");
   db_commit();
}

void       *db_query(enum db_query_res_type type, const char *fmt, ...) {
   va_list     ap;
   char       *buf;
   const char *tail;
   sqlite3_stmt *stmt;
   int         i = 0, cols;
   void       *ret = NULL;
   pkg_inode_t *inode;

   if (!(buf = mem_alloc(SQL_BUFSIZE))) {
      Log(LOG_ERR, "%s: alloc: %d:%s", __FUNCTION__, errno, strerror(errno));
      return NULL;
   }

   va_start(ap, fmt);
   vsnprintf(buf, SQL_BUFSIZE, fmt, ap);
   va_end(ap);

   if (dconf_get_bool("debug.db", 0) == 1)
      Log(LOG_DEBUG, "SQL query [%d]: %s ", type, buf);

   if ((i = sqlite3_prepare_v2(sqlite_db, buf, strlen(buf), &stmt, &tail) != SQLITE_OK)) {
      Log(LOG_WARNING, "SQL error: %s", sqlite3_errmsg(sqlite_db));
      mem_free(buf);
      errno = -i;
      return NULL;
   }

   while (sqlite3_step(stmt) == SQLITE_ROW) {
      cols = sqlite3_column_count(stmt);

      switch (type) {
         case QUERY_NULL:
            return (void *)0;
         case QUERY_INT:
            ret = (void *)sqlite3_column_int(stmt, 0);
            break;
         case QUERY_CHAR:
            ret = (void *)sqlite3_column_text(stmt, 0);
            break;
         case QUERY_INODE:
// This breaks thread safety assumptions. We need to keep
// a thread private BlockHeap or have a call to ask vfs for an inode
//
/*
            ret = (void *)blockheap_alloc(inode_heap);
            inode = (pkg_inode_t *) ret;

            while (i < cols) {
               const char *colname = sqlite3_column_name(stmt, i);

               if (!strcmp(colname, "gid"))
                  inode->st_gid = sqlite3_column_int64(stmt, i);
               else if (!strcmp(colname, "inode"))
                  inode->st_ino = sqlite3_column_int(stmt, i);
               else if (!strcmp(colname, "mode"))
                  inode->st_mode = sqlite3_column_int(stmt, i);
               else if (!strcmp(colname, "size"))
                  inode->st_size = sqlite3_column_int64(stmt, i);
               else if (!strcmp(colname, "time"))
                  inode->st_time = sqlite3_column_int64(stmt, i);
               else if (!strcmp(colname, "uid"))
                  inode->st_uid = sqlite3_column_int64(stmt, i);
               i++;
            }
*/
            break;
         default:
            break;
      }
   }

   sqlite3_finalize(stmt);
   mem_free(buf);
   return ret;
}

#undef	SQL_BUFSIZE

/* Open the database connection */
static int db_open(const char *path) {
   if (sqlite_db != NULL)
      return 0;

   if (path[0] != ':' && file_exists(path))
      unlink(path);

   if (sqlite3_open(path, &sqlite_db) != SQLITE_OK) {
      sqlite3_close(sqlite_db);
      Log(LOG_EMERG, "Error opening database %s", path);
      raise(SIGTERM);
   }

   // create the database structure, etc 
   db_initialize();

   return EXIT_SUCCESS;
}

static void db_close(void) {
   Log(LOG_DEBUG, "Closing database");
   sqlite3_close(sqlite_db);
}

/* Registers a package and returns its unique ID from the database */
int db_pkg_add(const char *path) {
   struct stat sb;

   // make sure the package still exists..sometimes they go away <bug://3371> 
   if (stat(path, &sb))
      return -errno;

   db_query(QUERY_NULL,
            "INSERT INTO packages (path, version) VALUES ('%s', %lu);", path, sb.st_mtime);
   return (u_int32_t) db_query(QUERY_INT,
                               "SELECT id FROM packages WHERE path = '%s' AND version = %lu;",
                               path, sb.st_mtime);
}

/* type: [f]ile, [d]ir, [l]ink, [p]ipe, f[i]fo, [c]har, [b]lock, [s]ocket */
int db_file_add(int pkg, const char *path, const char type,
                uid_t uid, gid_t gid, const char *owner, const char *group,
                size_t size, off_t offset, time_t ctime, mode_t mode,
                const char *perm) {
   db_query(QUERY_INT, "INSERT INTO files (package, path, type, uid, gid, owner, grp, size, offset, mode, ctime)"
                       "VALUES (%d, '%s', '%c', %i, %i, '%s', '%s', %lu, 0, %i, %lu);",
                       pkg, path, type, uid, gid, owner, group, size, offset, mode, ctime);
   return 0;
}

int db_pkg_remove(const char *path) {
   // If the package already exists (upgraded?), remove all traces of it 
   db_query(QUERY_NULL, "DELETE FROM files WHERE package IN (SELECT id FROM packages WHERE path = '%s');", path);
   db_query(QUERY_NULL, "DELETE FROM packages WHERE path = '%s';", path);
   return EXIT_SUCCESS;
}

int db_file_remove(int pkg, const char *path) {
   // delete any existing references to file with same path from same named pkg 
   db_query(QUERY_NULL, "DELETE FROM files WHERE package = '%lu' AND path = '%s';", pkg, path);
   return EXIT_SUCCESS;
}


void *thread_database_init(void *data) {
    thread_entry((dict *)data);

    Log(LOG_INFO, "Opening database %s", dconf_get_str("path.db", ":memory"));
    db_open(dconf_get_str("path.db", ":memory"));

    while (!conf.dying) {
        sleep(3);
        pthread_yield();
    }

    return NULL;
}

void *thread_database_fini(void *data) {
   db_close();
   thread_exit((dict *)data);
   return NULL;
}
