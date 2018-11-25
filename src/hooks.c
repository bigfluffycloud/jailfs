/*
 * Event Hook system
 *
 * There's lots of places you can leak memory here...
 * *ALL* strings, *ALL* structs ***MUST*** be allocated
 * as we cross thread-boundaries in the MLS/DPS code
 */
#include <errno.h>
#include "dict.h"
#include "hooks.h"
#include "logger.h"
#include "memory.h"
/* Be gentle with this, it can and will explode if you touch it wrong */
static dict *hooks = NULL;

Hook *hook_find(const char *name) {
   Hook *hp = NULL;

   if (!name)
      return NULL;

   /* Query for hook pointer */
   hp = (Hook *)dict_get_blob(hooks, name, NULL);

   return hp;
}

/*
 * Deallocate used memory and free children
 */
int hook_destroy_children(Hook *hook) {
   return 0;
}

int hook_destroy(const char *name) {
   Hook *hp;
   int rv = 0;

   if ((hp = hook_find(name)))
      return EFAULT;

   /* Cleanup dependants and allocated memory */
   hook_destroy_children(hp);
   dict_del(hooks, name);
   mem_free(hp);
   return rv;
}

Hook *hook_add(const char *name, int flags, HookFunc *func) {
   Hook *tmp = NULL;

   if (!name || !func)
      return tmp;

   if (!(tmp = hook_find(name))) {
      tmp = mem_alloc(sizeof(Hook));
      memset(tmp, 0, sizeof(Hook));
      tmp->flags = flags;
      if (name != NULL)
         tmp->name = strdup(name);
   }

   if (tmp != NULL)
      dict_add_blob(hooks, name, (void *)tmp);
   else
      Log(LOG_FATAL, "Failed registering hook %s");

   return tmp;
}
