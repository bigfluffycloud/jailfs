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
#if	!defined(__ax_profiling_h)
#define	__ax_profiling_h
#include <libunwind.h>
extern void profiling_dump(void);
extern void profiling_toggle(void);
extern int profiling_newmsg;
extern char profiling_msg[512];

static __inline__ void stack_unwind(void) {
    unw_cursor_t cursor;
    unw_word_t ip, sp;  
    unw_context_t uc;   
    unw_getcontext(&uc);
#if	0
    unw_init_local(&cursor, &uc);
    do
      {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);

        printf ("ip=%016lx sp=%016lx\n", ip, sp);
      }
    while (unw_step (&cursor) > 0);
#endif
}
#endif	// !defined(__ax_profiling_h)
