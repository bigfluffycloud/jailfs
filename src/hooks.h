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

#if	!defined(__hooks_h)
#define	__hooks_h

typedef struct Hook {
  int refcnt;			/* Reference count */
  int flags;
  char *name;			/* Pointer to alloc() string of name */
// XXX: Change this to dlink
//  list_p handlers;		/* Chain of handlers */
} Hook;

typedef dict *(*HookFunc)(const char *name, dict *args);

#define	HOOK_EXCLUSIVE	0x0001	/* Only one hook allowed */
#define	HOOK_ONESHOT	0x0002	/* Remove after triggered */

#endif	// !defined(__hooks_h)
