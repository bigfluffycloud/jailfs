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
#if	!defined(__module_h)
#define	__module_h
#include <lsd/dlink.h>
#include <lsd/dict.h>
#include <lsd/list.h>

#define	MODULE_MAGIC	0x49fa0000
#define	API_VER_V1	(MODULE_MAGIC | 0x1)
#define	API_MAGIC(x)	((x) & 0xffff0000)
#define	API_VER(x)	((x) & 0x0000ffff)

#define	MODULE_SUCCESS	0
#define	MODULE_FAILURE	1

/* xxx: This only works for text-like strings, a dictionary would be really nice */
typedef int (*ModuleHookFunc)(dict *args);

typedef struct	Module {
/* Header */
  char *name;
  const char *version;
  int core;
  int api_version;
  void *header;

/* Functions */
  int (*init)(dict *conf);
  int (*fini)(dict *conf);
/* Storage for system data */
  void *dlptr;
  char *path;
} Module;

/* this is a hack, to make the library easier to use */

extern list_p Modules;
extern int in_module;
extern int module_load(const char *path);
extern int module_unload(Module *mp);
extern int module_dying(int signal);
extern Module *module_find(const char *name);

#endif	// !defined(__module_h)
