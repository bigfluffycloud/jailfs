/*
 * Profiling support, modeled off ircd-bahamut and others
 *
 *    Determine which areas of AppWorx are using the most
 * RAM and CPU
 *
*    Talk to ax.profiling (See: AX_MSG:
 */
#include <sys/gmon.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "profiling.h"
#define	monstartup __monstartup

extern void _start,
            etext;
static int profiling_state = 1;
int profiling_newmsg = 0;
char profiling_msg[512];

void profiling_dump(void) {
   char buf[32];

   sprintf(buf, "gmod.%lu", time(NULL));
   setenv("GMON_OUT_PREFIX", buf, 1);
   _mcleanup();
   monstartup((unsigned long)&_start, (unsigned long)&etext);
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
      monstartup((unsigned long)&_start, (unsigned long)&etext);
      setenv("GMON_OUT_PREFIX", "gmon.auto", 1);
      profiling_state = 1;
      sprintf(profiling_msg, "Turned profiling ON");
   }

   profiling_newmsg = 1;
}