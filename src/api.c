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
 * src/api.c:
 *		Inter-thread API using dict objects
 *
 *	We try to provide a thread safe way to commnicate
 * across threads and dispatch commands.
 */
#include <lsd.h>
#include "conf.h"
#include "memory.h"
#include "shell.h"
#include "hooks.h"
#include "api.h"

static BlockHeap *heap_api_msg = NULL;

APImsg *api_create_message(const char *sender, const char *dest, const char *cmd) {
    APImsg *p = NULL;

    if (!(p = blockheap_alloc(heap_api_msg))) {
       Log(LOG_EMERG, "out of memory attempting to create API message. halting!");
       raise(SIGTERM);
    }

    // Create the command packet
    memset(p, 0, sizeof(p));
    memcpy(p->sender, sender, API_ADDR_MAX);
    memcpy(p->dest, dest, API_ADDR_MAX);
    memcpy(p->cmd, cmd, API_CMD_MAX);
    p->req = dict_new();
    p->res = dict_new();

    return p;
}

int api_destroy_message(APImsg *msg) {
    dict_free(msg->req);
    dict_free(msg->res);
    blockheap_free(heap_api_msg, msg);
    msg = NULL;
    return 0;
}

//////////////////////////
// main thread (master) //
//////////////////////////
int api_master_init(void) {
    heap_api_msg = blockheap_create(sizeof(APImsg), dconf_get_int("tuning.heap.api-msg", 512), "api messages");

    return 0;
}

int api_master_fini(void) {
    blockheap_destroy(heap_api_msg);
    heap_api_msg = NULL;

    return 0;
}

void api_gc(void) {
    blockheap_garbagecollect(heap_api_msg);
}

///////////////////////
// All other threads //
///////////////////////
int api_init(void) {
    // Do stuff and things

    // Return success
    return 0;
}

int api_fini(void) {
    // Do stuff and things

    // Return success
    return 0;
}
