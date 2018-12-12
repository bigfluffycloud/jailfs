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
/*
 * src/jail.c:
 *	Code we use to restrict the processes running inside of the
 * jail. Ideally, we will be running in a clean namespace, with no
 * privileges.
 */
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "conf.h"
#include "memory.h"
#include "str.h"
#include "logger.h"
#include "dict.h"
#include "threads.h"

// XXX: Detect what namespaces are enabled and use what we can
// CLONE_NEWUSER|CLONE_NEWCGROUP missing...
#define	CLONE_OPTIONS	CLONE_NEWIPC|CLONE_NEWNET|CLONE_NEWNS|CLONE_NEWPID|CLONE_NEWUTS|CLONE_FILES|CLONE_FS|CLONE_SYSVSEM

static int	jail_drop_privs(void) {
    Log(LOG_INFO, "[cell] dropping privileges...");
    return 0;
}

static int	jail_chroot(const char *path) {
    Log(LOG_INFO, "[cell] jail_chroot(): bound root to vfs at %s/%s", get_current_dir_name, path);
    
    return 0;
}


////////////////////
// Jail namespace //
////////////////////
/////
// thread main:cell
/////

static int	jail_namespace_init(void) {
    int rv = -1;

    
    if (rv = unshare(CLONE_OPTIONS)) {
       Log(LOG_EMERG, "unshare: %d: %s", errno, strerror(errno));
       raise(SIGTERM);
    }

    Log(LOG_DEBUG, "[cell] isolated from parent namespaces successfully (%lu)", CLONE_OPTIONS);
    return 0;
}

static void jail_env_init(void) {
}

void jail_container_launch(void) {
    char *init_cmd = dconf_get_str("init.cmd", "/init.sh");
    char *argv[] = { init_cmd, NULL };
    Log(LOG_DEBUG, "[cell] Launching container init %s", init_cmd);

    // Does it?
    Log(LOG_INFO, "[cell] init: inmate needs CAP_NET_BIND_SERVICE, trying to set...");
    // XXX: give the init process  CAP_NET_BIND_SERVICE
    // XXX: Which exec call do we want to use?
    if (execv(init_cmd, argv) == -1) {
       Log(LOG_EMERG, "[cell] init: Failure executing: %d '%s' - sleep 12 extra seconds for safety", errno, strerror(errno));
       sleep(12);
       return;
    }

    // For now just keep from cooking the CPU...
    while (!conf.dying) {
       sleep(3);
       pthread_yield();
   }
}

void *thread_cell_init(void *data) {
    thread_entry((dict *)data);

    // XXX: We need to wait for the main 'ready' signal
    sleep(1);

    // Detach from our controlling process...
    jail_chroot(dconf_get_str("path.mountpoint", NULL));
    jail_namespace_init();

    while (!conf.dying) {
       Log(LOG_INFO, "[cell] Preparing the jail cell for use...");

       if (chdir("/")) { /* avoid warning  */}

       // Set up the jail ENVironment
       jail_env_init();

       // Supervise the task
       Log(LOG_INFO, "[cell] init: a new inmate has arrived in the cell.");

       // Start the init command
       jail_container_launch();

       // If process unexpectedly dies, reset the container and start over...
       Log(LOG_NOTICE, "[cell] init: inmate has died, replacing him in 3 seconds!");
       sleep(3);
    }
    return NULL;
}

void *thread_cell_fini(void *data) {
    thread_exit((dict *)data);
    return NULL;
}
