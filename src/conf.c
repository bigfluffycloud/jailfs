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
 * Dictionary based configuration lookup
 * 	Used for single-entry configurations
 *	See lconf.[ch] for list-based configuration
 *	suitable for ACLs and similar options.
 * Copyright (C) 2008-2018 bigfluffy.cloud
 *
 * This code wouldn't be possible without N. Devillard's dictionary.[ch]
 * from the iniparser package. See dict.[ch] for slightly modified version
 * Thanks!!
 */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <strings.h>
#include "dict.h"
#include "logger.h"
#include "dlink.h"
#include "str.h"
#include "conf.h"
#include "timestr.h"
#include "memory.h"
#include "module.h"
#include "i18n.h"

#define	JAILCONF_SIZE	16384		// should be plenty...

struct conf conf;

void dconf_fini(void) {
   dict_mem_free(_CONF_DICT);
   _CONF_DICT = NULL;
}

void dconf_init(const char *file) {
   _CONF_DICT = dconf_load(file);
}

/////////////////
// in progress //
#define	CONF_SECT_KEYVAL	0x01
#define	CONF_SECT_DICT		0x02
#define	CONF_SECT_CUSTOM	0x04

struct ConfigFile {
   char section;
   int section_type;
};
/////////////////

dict *dconf_load(const char *file) {
   int line = 0, errors = 0, warnings = 0;
   int         in_comment = 0;
   char buf[768];
   FILE *fp;
   char *end, *skip,
        *key, *val,
        *section = NULL;
   dict *cp = dict_new();

   if (!(fp = fopen(file, "r"))) {
      Log(LOG_ERR, "%s: Failed loading %s", __FUNCTION__, file);
      fclose(fp);
      return false;
   }

   Modules = create_list();

   //
   // This could use some cleanup... But it does work.
   //
   // We need to use safer string functions...
   //
   do {
      memset(buf, 0, sizeof(buf));
      char *discard = fgets(buf, sizeof(buf) - 1, fp);
      line++;

      // delete prior whitespace...
      skip = buf;
      while(*skip == ' ')
        skip++;

      // Delete trailing newlines
      end = buf + strlen(buf);
      do {
        *end = '\0';
        end--;
      } while(*end == '\r' || *end == '\n');

      // did we eat the whole line?
      if ((end - skip) <= 0)
         continue;


      if (skip[0] == '*' && skip[1] == '/') {
         in_comment = 0;               /* end of block comment */
         continue;
      } else if (skip[0] == ';' || skip[0] == '#' || (skip[0] == '/' && skip[1] == '/')) {
         continue;                     /* line comment */
      } else if (skip[0] == '/' && skip[1] == '*')
         in_comment = 1;               /* start of block comment */

      if (in_comment)
         continue;                     /* ignored line, in block comment */

      if ((*skip == '/' && *skip+1 == '/') ||		// comments
           *skip == '#' || *skip == ';')
         continue;
      else if (*skip == '[' && *end == ']') {		// section
         section = strndup(skip + 1, strlen(skip) - 2);
         Log(LOG_DEBUG, "cfg.section.open: '%s'", section);
         continue;
      } else if (*skip == '@') {			// preprocessor
         if (strncasecmp(skip + 1, "if ", 3) == 0) {
            /* XXX: finish this */
         } else if (strncasecmp(skip + 1, "endif", 5) == 0) {
            /* XXX: finish this */
         } else if (strncasecmp(skip + 1, "else ", 5) == 0) {
            /* XXX: finish this */
         } else if (strncasecmp(skip + 1, "include ", 8) == 0) {
            /* XXX: Add includes (non-recursive?) */
         }
      }

      // Configuration data *MUST* be inside of a section, no exceptions.
      if (!section) {
         Log(LOG_WARNING, "config %s:%d: line outside of section: %s", file, line, buf);
         warnings++;
         continue;
      }

      // Handle configuration sections
      if (strncasecmp(section, "general", 7) == 0) {
         // Parse configuration line (XXX: GET RID OF STRTOK!)
         key = strtok(skip, "= \n");
         val = strtok(NULL, "= \n");

         // Store value
         dict_add(cp, key, val);
      } else if (strncasecmp(section, "modules", 8) == 0) {
         Log(LOG_DEBUG, "Loading module %s", skip);

         if (module_load(skip) != 0)
            errors++;
      } else if (strncasecmp(section, "jail", 4) == 0) {
         // We ignore [jail] ection as it is for warden
         if (strncasecmp(skip, "@END", 4) == 0)
            section = NULL;
         continue;
      } else if (strncasecmp(section, "language", 8) == 0) {
         // Parse configuration line (XXX: GET RID OF STRTOK!)
         key = strtok(skip, "= \n");
         val = strtok(NULL, "= \n");
         if (strncasecmp(key, "name", 4) == 0) {
           // XXX:
         } else if (strncasecmp(key, "description", 12) == 0) {
           // XXX:
         } else if (strncasecmp(key, "author", 6) == 0) {
           // XXX:
         }
      } else if (strncasecmp(section, "strings", 7) == 0) {
         // XXX: Store i18n strings in a dictionary
      } else {
         Log(LOG_WARNING, "Unknown configuration section '%s' parsing '%s' at %s:%d", section, buf, file, line);
         warnings++;
      }
   } while (!feof(fp));

   Log(LOG_INFO, "configuration loaded with %d errors and %d warnings from %s (%d lines)", errors, warnings, file, line);
   fclose(fp);

   return cp;
}

// Write-out the in-memory configuration
int dconf_write(dict *cp, const char *file) {
   FILE *fp;
   time_t t;
   int errors = 0;
   char nambuf[PATH_MAX];

   sprintf(nambuf, "%s.auto", file);

   if ((fp = fopen(nambuf, "w+")) == NULL || errno) {
      Log(LOG_ERR, "%s: fopen(%s, \"w+\") failed: %d (%s)", __FUNCTION__, nambuf, errno, strerror(errno));

      if (fp)
         fclose(fp);
      return false;
   }

   t = time(NULL);

   // Dump configuration to file..
   if (fprintf(fp, "# This configuration file automatically saved on %s\n", ctime(&t)) < 0)
      errors++;

   if (fprintf(fp, "[general]\n") < 0)
      errors++;

   if (dict_dump(cp, fp) < 0)
      errors++;
#if	0	// XXX: Rewrite this using dlink or dict?
   if (Modules) {
      Module *mp = NULL;
      list_iter_p mod_cur = list_iterator(Modules, FRONT);

      fprintf(fp, "[modules]\n");

      do {
         if (mp->name && fprintf(fp, "%s\n", mp->name) < 0)
            errors++;
         fflush(fp);
      } while((mp = list_next(mod_cur)));
   } else
     Log(LOG_ERR, "conf_write: No Modules list");
#endif
   if (errors > 0)
      fprintf(fp, "# Saved with %d errors, please review before using, autorename disabled\n", errors);

   fclose(fp);

   if (errors == 0) {
      unlink(file);
      rename(nambuf, file);
      Log(LOG_INFO, "Saved configuration as %s with no errors", file);
   } else
      Log(LOG_ERR, "Saving configuration had %d errors, keeping as %s", errors, nambuf);

   return true;
}

int dconf_get_bool(const char *key, const int def) {
   char       *tmp;
   int         rv = 0;

   if ((tmp = dconf_get_str(key, NULL)) == NULL)
      return def;
   else if (strcasecmp(tmp, "true") == 0 || strcasecmp(tmp, "on") == 0 ||
            strcasecmp(tmp, "yes") == 0 || (int)strtol(tmp, NULL, 0) == 1)
      rv = 1;
   else if (strcasecmp(tmp, "false") == 0 || strcasecmp(tmp, "off") == 0 ||
            strcasecmp(tmp, "no") == 0 || (int)strtol(tmp, NULL, 0) == 0)
      rv = 0;

   return rv;
}

double dconf_get_double(const char *key, const double def) {
   char       *tmp;

   if ((tmp = dconf_get_str(key, NULL)) == NULL)
      return def;

   return atof(tmp);
}

int dconf_get_int(const char *key, const int def) {
   char       *tmp;

   if ((tmp = dconf_get_str(key, NULL)) == NULL)
      return def;

   return (int)strtol(tmp, NULL, 0);
}

char       *dconf_get_str(const char *key, const char *def) {
   if (_CONF_DICT == NULL || key == NULL)
      return NULL;

   return (char *)dict_get(_CONF_DICT, key, def);
}

time_t dconf_get_time(const char *key, const time_t def) {
   return (timestr_to_time(dconf_get_str(key, NULL), def));
}

int dconf_set(const char *key, const char *val) {
   return dict_add(_CONF_DICT, key, val);
}

void dconf_unset(const char *key) {
   dict_del(_CONF_DICT, key);
}
