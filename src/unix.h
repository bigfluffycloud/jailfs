#if	!defined(__unix_h)
#define	__unix_h

extern void ax_enable_coredump(void);
extern void ax_unlimit_fds(void);
extern void unix_init(void);
extern void host_init(void);
extern int pidfile_open(const char *path);
extern void host_detach(void);

#endif	/* !defined(__unix_h) */
