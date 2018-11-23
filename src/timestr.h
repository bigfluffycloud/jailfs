#if	!defined(__TIMESTRING)
#define	__TIMESTRING
#include <time.h>
extern time_t timestr_to_time(const char *str, const time_t def);
extern char *time_to_timestr(time_t itime);
#endif                                 /* !defined(__TIMESTRING) */
