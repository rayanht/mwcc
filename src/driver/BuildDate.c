#include "version.h"

/* The time and date the release was built (this file's __TIME__ and __DATE__ when it was). */
#if VERSION == VERSION_GC_1_0
char *build_time = "14:30:41";
char *build_date = "Apr 13 2000";
#elif VERSION < VERSION_GC_1_2_5
char *build_time = "12:08:38";
char *build_date = "Feb  7 2001";
#elif VERSION < VERSION_GC_1_3
char *build_time = "10:58:30";
char *build_date = "Apr 23 2001";
#else
char *build_time = "17:24:36";
char *build_date = "Jan 10 2002";
#endif
