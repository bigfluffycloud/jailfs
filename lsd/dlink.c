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
 * Sentinel - IRC Statistical and Operator Services
 * dlink.c - Hybrid derived doubly linked list library
 *
 * Copyright W. Campbell and others.  See README for more details
 * Some code Copyright: Jonathan George, Kai Seidler, ircd-hybrid Team,
 *                      IRCnet IRCD developers.
 *
 * $Id: dlink.c,v 1.2 2003/09/11 14:56:34 wcampbel Exp $
 *
 *  tcm-hybrid/src/tools.c by fl_
 *  Copyright (C) 2002 ircd-hybrid development team
 *
 *  #Id: tools.c,v 1.6 2002/06/24 16:21:51 leeh Exp #
 */
#include <sys/signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include "balloc.h"
#include "conf.h"
#include "dlink.h"
int         dlink_count = 0;
BlockHeap  *dlink_node_heap;

void dlink_init(void) {
   if (!(dlink_node_heap = blockheap_create(sizeof(dlink_node), dconf_get_int("tuning.heap.node", 1024), "dlink_node"))) {
      fprintf(stderr, "dlink_init(): block allocator failed\n");
      raise(SIGSEGV);
   }
}

void dlink_fini(void) {
   blockheap_destroy(dlink_node_heap);
}

dlink_node *dlink_create(void) {
   dlink_node *m;
   m = (dlink_node *) blockheap_alloc(dlink_node_heap);
   m->data = m->next = m->prev = NULL;

   dlink_count++;

   return m;
}

/* XXX - macro? */
void dlink_free(dlink_node * m) {
   dlink_count--;
   blockheap_free(dlink_node_heap, m);
}

void dlink_add(void *data, dlink_node * m, dlink_list * list) {
   m->data = data;
   m->next = list->head;
   m->prev = NULL;

   if (list->head != NULL)
      list->head->prev = m;
   else if (list->tail == NULL)
      list->tail = m;

   list->head = m;
   list->length++;
}

void dlink_add_before(dlink_node * b, void *data, dlink_node * m, dlink_list * list) {
   /*
    * Shortcut - if its the first one, call dlinkAdd only 
    */
   if (b == list->head) {
      dlink_add(data, m, list);
   } else {
      m->data = data;
      b->prev->next = m;
      m->prev = b->prev;
      b->prev = m;
      m->next = b;
      list->length++;
   }
}

void dlink_add_tail(void *data, dlink_node * m, dlink_list * list) {
   m->data = data;
   m->next = NULL;
   m->prev = list->tail;

   if (list->head == NULL)
      list->head = m;
   else if (list->tail != NULL)
      list->tail->next = m;

   list->tail = m;
   list->length++;
}

void dlink_delete(dlink_node * m, dlink_list * list) {
   /*
    * item is at head 
    */
   if (m->prev == NULL)
      list->head = m->next;
   else
      m->prev->next = m->next;

   /*
    * item is at tail 
    */
   if (m->next == NULL)
      list->tail = m->prev;
   else
      m->next->prev = m->prev;

   list->length--;
}

dlink_node *dlink_find(void *data, dlink_list * list) {
   dlink_node *ptr;

   DLINK_FOREACH(ptr, list->head) {
      if (ptr->data == data)
         return ptr;
   }

   return NULL;
}

dlink_node *dlink_find_delete(void *data, dlink_list * list) {
   dlink_node *ptr;

   DLINK_FOREACH(ptr, list->head) {
      if (ptr->data != data)
         continue;

      if (ptr->next != NULL)
         ptr->next->prev = ptr->prev;
      else
         list->tail = ptr->prev;

      if (ptr->prev != NULL)
         ptr->prev->next = ptr->next;
      else
         list->head = ptr->next;

      ptr->next = ptr->prev = NULL;

      list->length--;

      return ptr;
   }

   return NULL;
}

int dlink_length(dlink_list * list) {
   return list->length;
}

void dlink_move(dlink_node * m, dlink_list * oldlist, dlink_list * newlist) {
   if (m == NULL || oldlist == NULL || newlist == NULL)
      return;

   /*
    * Assumption: If m->next == NULL, then list->tail == m
    * *      and:   If m->prev == NULL, then list->head == m
    */
   if (m->next)
      m->next->prev = m->prev;
   else
      oldlist->tail = m->prev;

   if (m->prev)
      m->prev->next = m->next;
   else
      oldlist->head = m->next;

   m->prev = NULL;
   m->next = newlist->head;

   if (newlist->head != NULL)
      newlist->head->prev = m;
   else if (newlist->tail == NULL)
      newlist->tail = m;

   newlist->head = m;

   oldlist->length--;
   newlist->length++;
}
