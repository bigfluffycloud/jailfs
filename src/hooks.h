#if	!defined(__ax_hooks_h)
#define	__ax_hooks_h

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

#endif	// !defined(__ax_hooks_h)
