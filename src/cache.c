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

void *thread_cache_init(void *data) {
   char *cache = NULL;

   // Caching support
   cache = dconf_get_str("path.cache", NULL);
   if (strcasecmp("tmpfs", dconf_get_str("cache.type", NULL)) == 0) {
      if (cache != NULL) {
         int rv = -1;

         // If .keepme exists in cachedir (from git), remove it
         char tmppath[PATH_MAX];
         memset(tmppath, 0, sizeof(tmppath));
         snprintf(tmppath, sizeof(tmppath) - 1, "%s/.keepme", cache);
         if (file_exists(tmppath))
            unlink(tmppath);

         if ((rv = mount("jailfs-cache", cache, "tmpfs", 0, NULL)) != 0) {
            Log(LOG_ERR, "mounting tmpfs on cache-dir %s failed: %d (%s)", cache, errno, strerror(errno));
            exit(1);
         }
      }
   }

   return NULL;
}

void *thread_cache_fini(void *data) {
   return NULL;
}
