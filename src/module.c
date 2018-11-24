/* Dynamic modules support */
#include <dlfcn.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include "logger.h"
#include "list.h"
#include "module.h"
#include "util.h"
#include "memory.h"
list_p Modules;
int in_module = 0;
sigjmp_buf env;

/*
 * This MUST be called from each module_init() to setup things like Exceptions
 */
void *module_thread_init(void *arg) {
   Module *mp = (Module *)arg;

   in_module = 1;

   // Restart the module if it crashes
   if (sigsetjmp(env, 1) == 0)
      (mp->init)(NULL);

   return NULL;
}

int module_load(const char *path) {
   Module *tmp;
   void *dlptr;

   if (!is_file(path)) {
      Log(LOG_ERROR, "Module %s doesn't exist, skipping", path);
      return ENOENT;
   }

   if ((dlptr = dlopen(path, RTLD_LAZY|RTLD_GLOBAL)) == NULL) {
      Log(LOG_ERROR, "Failed dlopen()ing module %s: %s", path, dlerror());
      return EBADF;
   }

   if ((tmp = (Module *)dlsym(dlptr, "module")) == NULL) {
      Log(LOG_ERROR, "Failed finding 'module' symbol in %s: %s", path, dlerror());
      return EBADF;
   }

   /* Store local data into module structure */
   tmp->path = strdup(path);
   tmp->dlptr = dlptr;

   if (tmp->init)
      (tmp->init)(conf.dict);

   list_add(Modules, tmp, sizeof(tmp));
   return 0;
}

int module_unload(Module *mp) {
   if (!mp) {
      Log(LOG_DEBUG, "module_unload: called with invalid mp");
      return 1;
   }

   /* If destructor exists, call it */
   if (mp->fini)
      (mp)->fini(conf.dict);

   /* If dlopen handle, close it */
   if (mp->dlptr)
      dlclose(mp->dlptr);

   /* If module path, free it */
   if (mp->path)
      mem_free(mp->path);

   return 0;
}

int module_dying(int signal) {
   if (signal == SIGSEGV)
      siglongjmp(env, -1);

   return 0;
}


Module *module_find(const char *name) {
  Module *mp = NULL;
  list_iter_p mod_cur = list_iterator(Modules, FRONT);

  do {
     if (mp && strcasecmp(mp->name, name) == 0)
        return mp;
  } while ((mp = list_next(mod_cur)));

  return NULL;
}
