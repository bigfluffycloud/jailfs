#if	!defined(__unix_h)
#define	__unix_h

extern void ax_enable_coredump();
extern void ax_unlimit_fds();
extern void unix_init();

#endif	/* !defined(__unix_h) */
