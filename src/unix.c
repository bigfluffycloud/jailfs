#include <sys/time.h>
#include <sys/resource.h>
#include "logger.h"
#include "unix.h"
#include "conf.h"
struct rlimit rl;

void enable_coredump(void) {
   rl.rlim_max = RLIM_INFINITY;
   setrlimit(RLIMIT_CORE, &rl);
}

void unlimit_fds(void) {
   rl.rlim_max = RLIM_INFINITY;
   setrlimit(RLIMIT_NOFILE, &rl);
}

void unix_init(void) {
   if (conf.log_level == LOG_DEBUG)
      Log(LOG_DEBUG, "enabling coredumps and raising fd limit");
   enable_coredump();
   unlimit_fds();
}

void	host_init(void) { unix_init(); }
