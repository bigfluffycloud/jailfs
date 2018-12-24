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
 * debugger.c:
 *	Extends the shell(.c) to add a debuggger
 */
#if	defined(CONFIG_DEBUGGER)
#include <sys/gmon.h>
#include <lsd/lsd.h>
#include "debugger.h"
#include "shell.h"
#include "conf.h"
#include "cron.h"
#include "hooks.h"
#include "i18n.h"
#include "linenoise.h"
#include "module.h"
#include "threads.h"
#include "unix.h"
#define	monstartup __monstartup

extern void *_start,
            *etext;
static int profiling_state = 1;
int profiling_newmsg = 0;
char profiling_msg[512];

// Symtab is prepared using nm and some filters by the GNUmakefile
// Example: vfs_spill_rmdir T /opt/jailfs/src/vfs_spill.c:60
struct symtab_ent {
    char	sym_name[128];
    char	sym_type;
    char	sym_file[PATH_MAX];
    u_int32_t	sym_line;
};
typedef struct symtab_ent symtab_ent_t;

symtab_ent_t	*debug_symtab_ent(symtab_ent_t *rv, char *line) {
    char *_name,
         *_type,
         *_file,
         *_line;

    if ((void *)rv == NULL) {
       errno = EINVAL;
       return NULL;
    }

    memset((void *)&rv, 0, sizeof(rv));

    // Parse the line:
    _name = line;
    _type = strchr(line, ' ') + 1;
    _file = strchr(_type, ' ') + 1;
    _line = strchr(_file, ':') + 1;

    memcpy(rv->sym_name, _name, sizeof(rv->sym_name));
    rv->sym_type = _type;
    *rv->sym_file = &_file;
    rv->sym_line = atoi(_line);

    return rv;
}

const char *debug_symtab_lookup(const char *symbol, const char *symtab) {
    char *st;
    FILE *fp;
    int line = 0;
    char buf[768];
    char *skip = buf;

    if (symtab == NULL) {
       Log(LOG_DEBUG, "symtab_lookup: Using default symtab from config: %s", dconf_get_str("path.symtab", NULL));
       st = dconf_get_str("path.symtab", "dbg/jailfs.symtab");
    } else {
       Log(LOG_DEBUG, "symtab_lookup: Using symtab from caller: %s", symtab);
       st = (char *)symtab;
    }

    if (file_exists(st) == 0) {
       Log(LOG_ERR, "symtab_lookup: Cannot find symtab: %s/%s", get_current_dir_name(), st);
       return NULL;
    }

    if (!(fp = fopen(st, "r"))) {
       Log(LOG_ERR, "symtab_lookup: Error reading from symtab %s: %d (%s)", st, errno, strerror(errno));
       fclose(fp);
       return NULL;
    }

    do {
       memset(buf, 0, sizeof(buf));
       char *discard = fgets(buf, sizeof(buf) - 1, fp);
       char *end = buf + strlen(buf);
       line++;

       // trim whitespace
       while (*skip == ' ')
         skip++;

       // trim trailing newlines and whitespace
       do {
          *end = '\0';
          end--;
       } while (*end == '\r' || *end == '\n' || *end == ' ');

       // did we consume the whole string?
       if ((end - skip) <= 0)
          continue;

       // Comments
       if (skip[0] == '#' || (skip[0] == '/' && skip[1] == '/') || skip[0] == ';')
          continue;

       Log(LOG_DEBUG, "read symtab line: %s", skip);
    } while(!feof(fp));

    return skip;
}

#endif	// defined(CONFIG_DEBUGGER)

#if	defined(CONFIG_PROFILING)
void profiling_dump(void) {
   char buf[32];

   sprintf(buf, "gmod.%lu", time(NULL));
   setenv("GMON_OUT_PREFIX", buf, 1);
   _mcleanup();
   monstartup(_start, etext);
   setenv("GMON_OUT_PREFIX", "gmon.auto", 1);
   sprintf(profiling_msg, "Reset profile, saved past profiling data to %s", buf);
   profiling_newmsg = 1;
}

void profiling_toggle(void) {
   char buf[32];

   if (profiling_state == 1) {
      sprintf(buf, "gmod.%lu", time(NULL));
      setenv("GMON_OUT_PREFIX", buf, 1);
      _mcleanup();
      sprintf(profiling_msg, "Turned profiling OFF, saved profiling data to %s", buf);
      profiling_state = 0;
   } else {
      monstartup((unsigned long)&_start, (unsigned long)etext);
      setenv("GMON_OUT_PREFIX", "gmon.auto", 1);
      profiling_state = 1;
      sprintf(profiling_msg, "Turned profiling ON");
   }

   profiling_newmsg = 1;
}
#endif	// defined(CONFIG_PROFILING)
