/*
 * tk/servers/fs-pkg:
 *	Package filesystem. Allows mounting various package files as
 *   a read-only (optionally with spillover file) overlay filesystem
 *   via FUSE or (eventually) LD_PRELOAD library.
 *
 * Copyright (C) 2012-2018 BigFluffy.Cloud <joseph@bigfluffy.cloud>
 *
 * Distributed under a BSD license. Send bugs/patches by email or
 * on github - https://github.com/bigfluffycloud/fs-pkg/
 *
 * No warranty of any kind. Good luck!
 */
/* Sentinel - IRC Statistical and Operator Services
** support.h - For systems that do not provide these functions
**
** Copyright W. Campbell and others.  See README for more details
** Some code Copyright: Jonathan George, Kai Seidler, ircd-hybrid Team,
**                      IRCnet IRCD developers.
**
** $Id: support.h,v 1.3 2003/09/29 17:09:29 wcampbel Exp $
*/

#ifndef SUPPORT_H
#define SUPPORT_H
#include <sys/types.h>
/* Fixes for broken operating systems */
#ifndef HAVE_STRLCAT
size_t      strlcat(char *, const char *, size_t);
#endif

#ifndef HAVE_STRLCPY
size_t      strlcpy(char *, const char *, size_t);
#endif

#ifndef HAVE_INET_PTON
int         inet_pton(int, const char *, void *);
#endif

#ifndef HAVE_INET_NTOP
const char *inet_ntop(int, const void *, char *, size_t);
#endif

#if 0
#ifndef AF_INET6
#define AF_INET6 28
#endif
#endif

#endif
