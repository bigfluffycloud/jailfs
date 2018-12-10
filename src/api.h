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
 * src/api.h:
 *		Inter-thread API using dict objects
 */
#if	!defined(__API_H)
#define	__API_H

#define	API_VERSION		0x0001
#define	API_CMD_MAX		64
#define	API_ADDR_MAX		64

// Request flags
#define	API_F_REPLY		0x0001	// Reply (non-error) to requet
#define	API_F_REQ_ERROR		0x0002	// Request contained an error
#define	API_F_ERROR		0x0004	// Error response from remote
#define	API_F_SECURITY		0x0008	// Security restriction denied request

struct API_Message {
   char 	sender[API_ADDR_MAX];
   char 	dest[API_ADDR_MAX];
   char 	cmd[API_CMD_MAX];   
   u_int32_t	flags;
   dict		*req,
   		*res;
};
typedef struct API_Message APImsg;

#endif	// !defined(__API_H)
