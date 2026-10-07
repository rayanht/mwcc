#ifndef MWCC_HOST_PROBE_ERRNO_H
#define MWCC_HOST_PROBE_ERRNO_H

/* MSL's Win32 errno: the first member of the thread's local data (lib/msl/MSL_Common/Include/cerrno,
   lib/msl/MSL_Win32/Include/ThreadLocalData.h). */
typedef struct {
    int errno;
} _ThreadLocalData;

_ThreadLocalData *_GetThreadLocalData(void);

#define errno (_GetThreadLocalData()->errno)

#endif
