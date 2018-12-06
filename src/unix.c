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
   if (conf.log_level == LOG_DEBUG)
      Log(LOG_DEBUG, "enabling coredumps and raising fd limit");

   enable_coredump();
   unlimit_fds();
}

void host_detach(void) {
   fprintf(stderr, "host_detach: Going into the background...\n");
   Log(LOG_INFO, "host_detach: Going into the background...");
   daemon(1, 1);
}

int	pidfile_open(const char *path) {
   FILE *fp = NULL;
   pid_t pid = 0;

   if (file_exists(path)) {
      errno = EADDRINUSE;
      return -1;
   }

   if ((fp = fopen(path, "w")) == NULL) {
      Log(LOG_FATAL, "pidfile_open: failed to open pid file %s: %d (%s)", path, errno, strerror(errno));
   }

   pid = getpid();
   fprintf(fp, "%i\n", pid);
   fflush(fp);
   fclose(fp);
   Log(LOG_DEBUG, "Wrote PID file %s: %d", path, pid);
   return 0;
}
