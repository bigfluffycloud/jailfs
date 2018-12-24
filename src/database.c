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
#include <sys/signal.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <lsd/lsd.h>
#include "conf.h"
#include "database.h"
#include "vfs.h"
#include "shell.h"

static pthread_mutex_t db_mutex;

/* transaction primitives */
void db_begin(void) {
   pthread_mutex_lock(&db_mutex);
}

void db_commit(void) {
   pthread_mutex_unlock(&db_mutex);
}

void db_rollback(void) {
   pthread_mutex_unlock(&db_mutex);
}

/* Registers a package and returns its unique ID from the database */
int db_pkg_add(const char *path) {
   struct stat sb;
   u_int32_t pkgid = -1;

   // make sure the package still exists..sometimes they go away <bug://3371> 
   if (stat(path, &sb))
      return -errno;

   return pkgid;
}

/* type: [f]ile, [d]ir, [l]ink, [p]ipe, f[i]fo, [c]har, [b]lock, [s]ocket */
int db_file_add(int pkg, const char *path, const char type,
                uid_t uid, gid_t gid, const char *owner, const char *group,
                size_t size, off_t offset, time_t ctime, mode_t mode,
                const char *perm) {
   return 0;
}

int db_pkg_remove(const char *path) {
   return EXIT_SUCCESS;
}

int db_file_remove(int pkg, const char *path) {
   return EXIT_SUCCESS;
}


void *thread_db_init(void *data) {
    thread_entry((dict *)data);

    while (!conf.dying) {
        sleep(3);
        pthread_yield();
    }

    return NULL;
}

void *thread_db_fini(void *data) {
   thread_exit((dict *)data);
   return NULL;
}
