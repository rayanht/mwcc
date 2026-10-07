#define CERROR_FILE "unknown.c"
#include "compiler/common.h"
#include "driver/CLLicenses.h"
#include "driver/CLErrors.h"
#include "driver/CLTarg.h"
#include "driver/MemUtils.h"
#include "driver/MsDos.h"
#include "driver/CLMain.h"
#include <string.h>

static struct License *data_0057ef08;
/* An opaque license value paired with its signed identifier. */
static UInt32 license_slots[32][2];
static int license_slot_count;
static int license_id_counter;

int allocate_license_slot(int licenseData, int negateId)
{
    int slot;

    slot = 0;
    if (0 < license_slot_count) {
        do {
            if (license_slots[slot][1] == 0)
                break;
            slot = slot + 1;
        } while (slot < license_slot_count);
    }
    if (slot >= 0x20) {
        release_negative_license_values();
        slot = license_slot_count;
    }
    if (slot < 0x20) {
        license_id_counter = license_id_counter + 1;
        license_slots[slot][0] = licenseData;
        license_slots[slot][1] = (negateId != 0) ? -license_id_counter : license_id_counter;
        if (slot >= license_slot_count) {
            license_slot_count = license_slot_count + 1;
        }
        return license_id_counter;
    }
    CLErrors_ReportInternalError("CLLicenses.c", 0x5b, "Out of license space");
    return 0;
}

int find_license(unsigned int identifier, unsigned int *license)
{
    int index;

    for (index = 0; index < license_slot_count; ++index) {
        if (identifier == license_slots[index][1] || identifier == -license_slots[index][1]) {
            *license = license_slots[index][0];
            return index;
        }
    }
    CLErrors_ReportInternalError("CLLicenses.c", 111, "Searched license not found");
    return -1;
}

int get_license_slot_values(int index, unsigned int *firstValue, int *secondValue)
{
    if (((0 <= index) && (index < license_slot_count)) && (license_slots[index][1] != 0)) {
        *firstValue = license_slots[index][0];
        *secondValue = license_slots[index][1];
        return 1;
    }
    return 0;
}

int delete_license(int licenseIndex)
{
    if (licenseIndex >= 0 && licenseIndex < license_slot_count) {
        license_slots[licenseIndex][1] = 0;
        license_slots[licenseIndex][0] = 0;
        if (licenseIndex + 1 == license_slot_count) {
            for (; licenseIndex >= 0 && license_slots[licenseIndex][1] == 0; --licenseIndex) {
                --license_slot_count;
            }
        }
        return 1;
    }
    CLErrors_ReportInternalError("CLLicenses.c", 0x94, "Deleted license not valid");
    return 0;
}

int get_license_slot_count(void)
{
    return license_slot_count;
}

void fn_00417750(void)
{
    license_slot_count = 0;
    data_0057ef08 = NULL;
    return;
}

void CLLicenses_ReleaseLicenses(void)
{
    int index;
    unsigned int firstValue;
    int count;
    release_negative_license_values();
    for (index = 0; index < (int)((long (*)(void))get_license_slot_count)(); index++) {
        if (((long (*)(int, unsigned int *, int *))get_license_slot_values)(index, &firstValue, &count) != 0 &&
            count > 0) {
            CLLicenses_DeleteLicense(count);
        }
    }
    if (data_0057ef08 != NULL) {
        memset(data_0057ef08, 0, 40);
        data_0057ef08 = NULL;
    }
}

int CLLicenses_RequestLicense(int request, int options, int cookieKind, char *errorMessage)
{
    char licensePath[256];
    MWInfo licenseInfo;
    int licenseHandle = 0;
    OSSpec alternatePath;
    int result = 0;
    int status;

    if (license_path != NULL) {
        strcpy(licensePath, license_path);
    } else {
        OSSpec *defaultPath;
        fn_00412340((defaultPath = &clState.programSpec)->directory.path, licensePath,
                    sizeof(defaultPath->directory.path));
        strcat(licensePath, "license.dat");
        if (OS_MakeFileSpec(licensePath, &alternatePath) != 0 || OS_Status(&alternatePath) != 0) {
            if (fn_004125b0(defaultPath, &alternatePath) == 0) {
                fn_00412340(alternatePath.directory.path, licensePath, sizeof(alternatePath.directory.path));
                strcat(licensePath, "license.dat");
            }
        }
    }
    if (data_0057ef08 == NULL) {
        data_0057ef08 = xmalloc(NULL, sizeof(struct License));
        data_0057ef08->type = 4;
        data_0057ef08->k4 = 0x4cf2d709;
        data_0057ef08->k8 = 0xd64a5128;
        data_0057ef08->k10 = 0x463ed9b0;
        data_0057ef08->kc = 0x745ce35b;
        data_0057ef08->k18 = 0xa610dfa5;
        data_0057ef08->k14 = 0xc304db20;
        data_0057ef08->w1c = 6;
        data_0057ef08->w1e = 1;
        data_0057ef08->b20 = 0;
        data_0057ef08->b21 = 0;
        strncpy(data_0057ef08->blob, "06.0", sizeof("06.0"));
    }
    licenseInfo.license = data_0057ef08;
    licenseInfo.vendor = "metrowks";
    status = lp_checkout(&licenseInfo, 0x101, request, options, 1, licensePath, &licenseHandle);
    strcpy(errorMessage, "No failure");
    if (status == 0) {
        result = allocate_license_slot(licenseHandle, cookieKind);
        if (result == 0) {
            strcpy(errorMessage, "Memory error:  Could not store license cookie");
            lp_checkin(licenseHandle);
        }
    } else {
        strcpy(errorMessage, lp_errstring(licenseHandle));
        lp_checkin(licenseHandle);
    }
    return result;
}

void CLLicenses_DeleteLicense(int identifier)
{
    int licenseIndex;
    unsigned int license;

    if (0 < identifier) {
        licenseIndex = find_license(identifier, &license);
        if (licenseIndex >= 0) {
            delete_license(licenseIndex);
            lp_checkin(license);
        }
    }
}

static inline int license_count(void)
{
    int (*count)(void) = get_license_slot_count;
    return count();
}

int release_negative_license_values(void)
{
    int index;
    unsigned int firstValue;
    int secondValue;

    for (index = 0; index < license_count(); index++) {
        int (*lookup)(int, unsigned int *, int *) = get_license_slot_values;
        if (lookup(index, &firstValue, &secondValue) && secondValue < 0) {
            void (*release)(int) = CLLicenses_DeleteLicense;
            release(secondValue);
        }
    }
}
