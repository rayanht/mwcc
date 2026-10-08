#ifndef VERSION_H
#define VERSION_H

#define VERSION_GC_1_0 0
#define VERSION_GC_1_1 1
#define VERSION_GC_1_1P1 2
#define VERSION_GC_1_2_5 3
#define VERSION_GC_1_2_5N 4
#define VERSION_GC_1_3 5

/* configure.py passes -DVERSION=<index> */
#ifndef VERSION
#define VERSION VERSION_GC_1_2_5
#endif

#endif
