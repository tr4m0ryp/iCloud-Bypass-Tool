/*
 * device.h -- Device detection and info querying via libimobiledevice.
 *
 * Extended from research_icloud0/src/device.h with CPID, ECID, MEID,
 * chip database integration, cellular detection, and DFU mode awareness.
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libusb-1.0/libusb.h>

#define DEVICE_FIELD_LEN 128
#define DEVICE_UDID_LEN  64
#define DEVICE_CHIP_NAME_LEN 32

typedef struct {
    /* Identification (existing) */
    char udid[DEVICE_UDID_LEN];
    char product_type[DEVICE_FIELD_LEN];   /* e.g. "iPad8,10"            */
    char chip_id[DEVICE_FIELD_LEN];        /* e.g. "0x8027" (string)     */
    char ios_version[DEVICE_FIELD_LEN];    /* e.g. "26.3"                */
    char activation_state[DEVICE_FIELD_LEN];
    char serial[DEVICE_FIELD_LEN];
    char imei[DEVICE_FIELD_LEN];
    char hardware_model[DEVICE_FIELD_LEN]; /* e.g. "J418AP"              */
    char device_name[DEVICE_FIELD_LEN];    /* user-assigned device name  */

    /* Handles kept open for service reuse. Caller must call device_free. */
    idevice_t           handle;
    lockdownd_client_t  lockdown;

    /* --- New fields (task 004) --- */

    /* Chip identification */
    uint32_t cpid;                             /* numeric chip ID (e.g. 0x8010)  */
    uint64_t ecid;                             /* exclusive chip ID              */
    char     meid[DEVICE_FIELD_LEN];           /* MEID for CDMA devices          */
    char     chip_name[DEVICE_CHIP_NAME_LEN];  /* "A10", "A11", etc. from chip_db */
    int      checkm8_vulnerable;               /* 1 if A5-A11, 0 if A12+         */

    /* Connectivity */
    int is_cellular;     /* 1 if IMEI or MEID present */
    int is_dfu_mode;     /* 0 in this module; set by usb_dfu */

    /* USB handle for DFU mode (NULL when not in DFU) */
    libusb_device_handle *usb;
} device_info_t;

/*
 * device_detect -- Find and connect to the first available USB device.
 * Populates dev->handle and dev->lockdown. Returns 0 on success, -1 on error.
 */
int device_detect(device_info_t *dev);

/*
 * device_query_info -- Query all device properties via lockdownd.
 * Requires device_detect() to have succeeded first.
 * Returns 0 on success, -1 on error (partial info may be populated).
 */
int device_query_info(device_info_t *dev);

/*
 * device_print_info -- Print all populated device fields to stdout.
 */
void device_print_info(const device_info_t *dev);

/*
 * device_free -- Release lockdown client and device handle.
 */
void device_free(device_info_t *dev);

#endif /* DEVICE_H */
