#include <sys/mount.h>
#include "dict.h"
#include "memory.h"
#include "balloc.h"
#include "conf.h"
#include "cron.h"
#include "logger.h"
#include "pkg.h"
#include "pkg_db.h"
#include "str.h"
#include "timestr.h"
#include "util.h"
#include "vfs.h"
#include "threads.h"
#include "balloc.h"

static pthread_mutex_t cache_mutex;
static char *cache_path = NULL;
static dict *cache_dict = NULL;
BlockHeap  *cache_entry_heap = NULL;

struct cache_entry {
   char 	jail_path[PATH_MAX];		// Path within the jail
   char		cache_path[PATH_MAX];		// Temporary file path
   u_int32_t	pkgid;
   u_int32_t	inode;
};
typedef struct cache_entry cache_entry_t;

// Thread constructor
void *thread_cache_init(void *data) {
   char *cache = NULL;

   thread_entry((dict *)data);
   cache = dconf_get_str("path.cache", NULL);
   cache_dict = dict_new();
   cache_entry_heap = blockheap_create(sizeof(cache_entry_t), dconf_get_int("tuning.heap.files", 1024), "cache entries");

   // If .keepme exists in cachedir (from git), remove it or mount will fail
   char tmppath[PATH_MAX];
   memset(tmppath, 0, sizeof(tmppath));
   snprintf(tmppath, sizeof(tmppath) - 1, "%s/.keepme", cache);
   if (file_exists(tmppath))
      unlink(tmppath);

   // Are we configured to use a tmpfs or host fs for cache?
   if (strcasecmp("tmpfs", dconf_get_str("cache.type", NULL)) == 0) {
      if (cache != NULL) {
         int rv = -1;

         // Attempt mounting tmpfs
         if ((rv = mount("jailfs-cache", cache, "tmpfs", 0, NULL)) != 0) {
            Log(LOG_ERR, "mounting tmpfs on cache-dir %s failed: %d (%s)", cache, errno, strerror(errno));
            exit(1);
         }
      }
   }

   // main loop
   while (!conf.dying) {
      sleep(3);
      pthread_yield();
   }

   return NULL;
}

// Thread destructor
void *thread_cache_fini(void *data) {
   dict_free(cache_dict);
   blockheap_destroy(cache_entry_heap);
   thread_exit((dict *)data);
   return NULL;
}

// Request a new file name
int cache_new_item(char *buf) {
    if (buf == NULL)
       return -1;

    return 0;
}
