#ifndef DRIVER_CLLICENSES_H
#define DRIVER_CLLICENSES_H

#include "compiler/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct License {
    UInt16 type;
    UInt16 rsv;
    UInt32 k4;
    UInt32 k8;
    UInt32 kc;
    UInt32 k10;
    UInt32 k14;
    UInt32 k18;
    UInt16 w1c;
    UInt16 w1e;
    UInt8 b20;
    UInt8 b21;
    char blob[5];
};
struct MWInfo {
    struct License *license;
    char *vendor;
};
/* The FLEXlm client library's (LMGR326B.dll, through its import library). */
extern int lp_checkout(struct MWInfo *info, int version, int request, int options, int flag, char *path, int *handle);
extern void lp_checkin(int handle);
extern char *lp_errstring(int handle);
extern int get_license_slot_values(int index, unsigned int *firstValue, int *secondValue);
extern int delete_license(int licenseIndex);
extern int find_license(unsigned int identifier, unsigned int *license);
extern int allocate_license_slot(int licenseData, int negateId);
extern int get_license_slot_count(void);
extern void fn_00417750(void);
extern int CLLicenses_RequestLicense(int request, int options, int cookieKind, char *errorMessage);
extern void CLLicenses_DeleteLicense(int identifier);
extern void CLLicenses_ReleaseLicenses(void);
extern int release_negative_license_values(void);
extern char *license_path;

#ifdef __cplusplus
}
#endif

#endif
