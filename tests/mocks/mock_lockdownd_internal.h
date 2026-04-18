/*
 * mock_lockdownd_internal.h -- Shared state for lockdownd + idevice mock.
 */

#ifndef TR4MPASS_MOCK_LOCKDOWND_INTERNAL_H
#define TR4MPASS_MOCK_LOCKDOWND_INTERNAL_H

#include "mock_internal.h"

#include <stddef.h>
#include <stdint.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <plist/plist.h>

struct idevice_private {
    char     udid[96];
    uint32_t magic;
};

struct lockdownd_client_private {
    char     label[64];
    uint32_t magic;
};

#define IDEV_MAGIC 0x49444556u   /* "IDEV" */
#define LCKD_MAGIC 0x4C434B44u   /* "LCKD" */

typedef struct {
    int    in_use;
    int    consumed;
    char   domain[64];       /* "" means match-any */
    char   key[64];
    char   type[16];         /* "string","uint","data","bool" */
    void  *data;
    size_t len;
} mock_lockdownd_stage_t;

/* State shared between _ctl.c and _sym.c. */
extern mock_lockdownd_stage_t g_lockdownd_stage[MOCK_STAGE_SLOTS];
extern int g_idevice_list_count;
extern int g_handshake_error;

mock_lockdownd_stage_t *find_lockdownd_stage(const char *domain,
                                             const char *key);

#endif /* TR4MPASS_MOCK_LOCKDOWND_INTERNAL_H */
