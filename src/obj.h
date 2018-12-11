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
#if	!defined(__obj_h)
#define	__obj_h
#include "uuid.h"
#include "dict.h"

typedef struct UUID {
  unsigned long hi,
                lo;
} UUID;

/* Representation of an object */
typedef struct obj {
  UUID	*uuid;
  dict	*values;
  dict	*attr;
} obj;


/* Object attributes */
extern int obj_attr_set(obj *o, const char *attr, const char *value);
extern int obj_attr_del(obj *o, const char *attr);
extern char *obj_attr_get(obj *o, const char *attr);

/* Object management, pass either UUID or object structure */
extern obj *obj_create(void);
extern int obj_del(UUID *uuid, obj *o);
extern obj *obj_find(UUID *uuid);
extern obj *obj_copy(UUID *uuid, obj *o); /* Create a NEW object, populated with data from old */
extern obj *obj_replicate(UUID *uuid, obj *o); /* Create a LINKED clone of the object */

//extern List *obj_attr_enum(obj *o);

#endif	/* !defined(__obj_h) */
