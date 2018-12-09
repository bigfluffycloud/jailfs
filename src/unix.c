#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include "logger.h"
#include "unix.h"
#include "conf.h"
#include "util.h"

struct rlimit rl;

void enable_coredump(void) {
   rl.rlim_max = RLIM_INFINITY;
   setrlimit(RLIMIT_CORE, &rl);
}

void unlimit_fds(void) {
   rl.rlim_max = RLIM_INFINITY;
   setrlimit(RLIMIT_NOFILE, &rl);
}

void host_init(void) {
   enable_coredump();
   unlimit_fds();
}

int	pidfile_open(const char *path) {
   FILE *fp = NULL;
   pid_t pid = 0;

   if (file_exists(path)) {
      errno = EADDRINUSE;
      return -1;
   }

   if ((fp = fopen(path, "w")) == NULL) {
      Log(LOG_EMERG, "pidfile_open: failed to open pid file %s: %d (%s)", path, errno, strerror(errno));
   }

   pid = getpid();
   fprintf(fp, "%i\n", pid);
   fflush(fp);
   fclose(fp);
   Log(LOG_DEBUG, "Wrote PID file %s: %d", path, pid);
   return 0;
}
