#include <sys/mount.h>
#include "dict.h"
#include "memory.h"
#include "balloc.h"
#include "conf.h"
#include "cron.h"
#include "logger.h"
#include "mimetypes.h"
#include "pkg.h"
#include "pkg_db.h"
#include "str.h"
#include "timestr.h"
#include "util.h"
#include "vfs.h"
#include "threads.h"

static pthread_mutex_t cache_mutex;
static char *cache_path = NULL;
static dict *cache_dict = NULL;

// Thread constructor
void *thread_cache_init(void *data) {
   char *cache = NULL;

   thread_entry((dict *)data);
   cache = dconf_get_str("path.cache", NULL);
   cache_dict = dict_new();

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

   thread_exit((dict *)data);
   return NULL;
}
