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
 *
 *
 * src/jail.c:
 *	Code we use to restrict the processes running inside of the
 * jail. Ideally, we will be running in a clean namespace, with no
 * privileges.
 */
#include <lsd.h>
#include "conf.h"
#include "shell.h"
#include "threads.h"

// XXX: Detect what namespaces are enabled and use what we can
// CLONE_NEWUSER|CLONE_NEWCGROUP missing...
#define	CLONE_OPTIONS	CLONE_NEWIPC|CLONE_NEWNET|CLONE_NEWNS|CLONE_NEWPID|CLONE_NEWUTS|CLONE_FILES|CLONE_FS|CLONE_SYSVSEM

static int	jail_drop_privs(void) {
    Log(LOG_INFO, "[cell] dropping privileges...");
    return 0;
}

static int	jail_chroot(const char *path) {
    if (chdir("/")) {  /* Nothing */ }
    Log(LOG_INFO, "[cell] jail_chroot(): bound root to vfs at %s/%s", get_current_dir_name(), path);
    
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
    char *hostname = dconf_get_str("jail.hostname", NULL);
    char oldhost[HOST_NAME_MAX];

    if (rv = unshare(CLONE_OPTIONS)) {
       Log(LOG_EMERG, "unshare: %d: %s", errno, strerror(errno));
       raise(SIGTERM);
    }

    memset(oldhost, 0, sizeof(oldhost));
    gethostname(oldhost, HOST_NAME_MAX);
    Log(LOG_INFO, "[cell] setting hostname %s (old: %s)", hostname, oldhost);

    if (sethostname(hostname, strlen(hostname)) != 0)
       Log(LOG_NOTICE, "[cell] failed to set hostname: %d: %s", errno, strerror(errno));

    Log(LOG_DEBUG, "[cell] isolated from parent namespaces successfully (%lu)", CLONE_OPTIONS);
    return 0;
}

// Environment should be empty...
static void jail_env_init(void) {
    Log(LOG_DEBUG, "[cell] setting default environment");
}

void jail_container_launch(void) {
    char *init_cmd = dconf_get_str("init.cmd", "/init.sh");
    char *argv[] = { init_cmd, NULL };
    // Does it?
//    Log(LOG_INFO, "[cell] init: inmate needs CAP_NET_BIND_SERVICE, trying to set...");
    // XXX: give the init process  CAP_NET_BIND_SERVICE
//    Log(LOG_DEBUG, "[cell] launching container init %s", init_cmd);

    // call clone to create another process

    // XXX: Which exec call do we want to use?
    if (execv(init_cmd, argv) == -1) {
//       Log(LOG_EMERG, "[cell] init: Failure executing: %d '%s' - sleep 12 extra seconds for safety", errno, strerror(errno));
       sleep(12);
       return;
    }
}


// This is what runs in the freshly created (isolated) process space after clone();
void *inmate_init(void) {
    jail_namespace_init();
}

void *thread_cell_init(void *data) {
    thread_entry((dict *)data);

    // XXX: We need to wait for the main 'ready' signal
    sleep(1);

    // Detach from our controlling process...
    Log(LOG_INFO, "[cell] Preparing the jail cell for use...");
    jail_chroot(dconf_get_str("path.mountpoint", NULL));

    while (!conf.dying) {
       // Set up the jail ENVironment
       jail_env_init();

       // Supervise the task
//       Log(LOG_INFO, "[cell] init: a new inmate has arrived in the cell.");

       // Start the init command
       jail_container_launch();

       // If process unexpectedly dies, reset the container and start over...
//       Log(LOG_NOTICE, "[cell] init: inmate has died, replacing him in 3 seconds!");
       sleep(3);
    }
    return NULL;
}

void *thread_cell_fini(void *data) {
    thread_exit((dict *)data);
    return NULL;
}
