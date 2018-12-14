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
// Host headers
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/mount.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <ev.h>

// LSD headers
#include <lsd/memory.h>
#include <lsd/dict.h>
#include <lsd/atomicio.h>
#include <lsd/balloc.h>
#include <lsd/dlink.h>
#include <lsd/list.h>
#include <lsd/str.h>
#include <lsd/timestr.h>
#include <lsd/tree.h>
#include <lsd/util.h>
