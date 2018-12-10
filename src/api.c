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
#include <sys/signal.h>
#include "conf.h"
#include "memory.h"
#include "logger.h"
#include "balloc.h"
#include "api.h"

static BlockHeap *api_msg_heap = NULL;

APImsg *api_create_message(const char *sender, const char *dest, const char *cmd) {
    APImsg *p = NULL;

    if (!(p = blockheap_alloc(api_msg_heap))) {
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
    blockheap_free(api_msg_heap, msg);
    msg = NULL;
    return 0;
}

int api_init(void) {
    api_msg_heap = blockheap_create(sizeof(APImsg), dconf_get_int("tuning.heap.api-msg", 512), "api messages");

    return 0;
}

int api_fini(void) {
    blockheap_destroy(api_msg_heap);
    api_msg_heap = NULL;

    return 0;
}
