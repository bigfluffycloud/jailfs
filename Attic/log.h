#if	!defined(__log_h)
#define	__log_h
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#define	LOG_INVALID_STR "*invalid_level*"

typedef struct {
  // private (do not modified
  enum { NONE = 0, LOG_syslog, LOG_stderr, LOG_file, LOG_fifo } type;
  FILE *fp;
  
} LogHndl;

extern void Log(int level, const char *msg, ...);
extern void log_open(const char *target);
extern void log_close(void);
extern LogHndl *mainlog;

#endif	// !defined(__log_h)
