/*
 * tk/servers/fs-pkg:
 *	Package filesystem. Allows mounting various package files as
 *   a read-only (optionally with spillover file) overlay filesystem
 *   via FUSE or (eventually) LD_PRELOAD library.
 *
 * Copyright (C) 2012-2018 BigFluffy.Cloud <joseph@bigfluffy.cloud>
 *
 * Distributed under a MIT license. Send bugs/patches by email or
 * on github - https://github.com/bigfluffycloud/fs-pkg/
 *
 * No warranty of any kind. Good luck!
 */
/*
 * src/jail.c:
 *	Code we use to restrict the processes running inside of the
 * jail. Ideally, we will be running in a clean namespace, with no
 * privileges.
 */

#include "conf.h"
#include "memory.h"
#include "str.h"
#include "logger.h"

int	jail_drop_privs(void) {
    Log(LOG_INFO, "jail_drop_privs(): called.");
    return 0;
}

int	jail_chroot(const char *path) {
    Log(LOG_INFO, "jail_chroot(): entered chroot %s", path);
    return 0;
}

int	jail_namespace_init(void) {
    return 0;
}
