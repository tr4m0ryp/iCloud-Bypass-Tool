/*
 * record.c -- Activation record plist builder.
 *
 * Constructs the activation record plists that the device needs to
 * transition to "Activated" state. Contains AccountToken,
 * DeviceCertificate, FairPlayKeyData, UniqueDeviceCertificate, and
 * related fields.
 *
 * Certificate data is populated with TODO placeholders -- in a real
 * flow these come from the device or are generated from device keys.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "activation/record.h"
#include "util/log.h"
#include "util/plist_helpers.h"

#define LOG_TAG "[record]"

/* --- Internal: AccountToken builder --- */

/*
 * Build the base64 AccountToken blob. In practice this contains
 * device URLs, IMEI, serial, and product type encoded as XML.
 * For offline bypass we build a minimal valid structure.
 */
static plist_t build_account_token(const device_info_t *dev)
{
    char token_buf[2048];
    int len;

    len = snprintf(token_buf, sizeof(token_buf),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "  <key>InStoreActivation</key>\n"
        "  <false/>\n"
        "  <key>IMEI</key>\n"
        "  <string>%s</string>\n"
        "  <key>MEID</key>\n"
        "  <string>%s</string>\n"
        "  <key>SerialNumber</key>\n"
        "  <string>%s</string>\n"
        "  <key>UniqueDeviceID</key>\n"
        "  <string>%s</string>\n"
        "  <key>ProductType</key>\n"
        "  <string>%s</string>\n"
        "</dict>\n"
        "</plist>",
        dev->imei, dev->meid, dev->serial,
        dev->udid, dev->product_type);

    if (len < 0 || (size_t)len >= sizeof(token_buf)) {
        log_error("%s AccountToken buffer overflow", LOG_TAG);
        return NULL;
    }

    /* AccountToken is stored as data (base64 in XML plist) */
    return plist_new_data(token_buf, (uint64_t)len);
}

/* --- Internal: common record fields --- */

/*
 * Populate the shared fields present in both standard and A12+ records.
 * Returns 0 on success, -1 on error.
 */
static int populate_common_fields(plist_t record, const device_info_t *dev)
{
    plist_t token = NULL;

    if (!record || !dev) return -1;

    /* AccountToken */
    token = build_account_token(dev);
    if (!token) {
        log_error("%s Failed to build AccountToken", LOG_TAG);
        return -1;
    }
    plist_dict_set_item(record, "AccountToken", token);

    /*
     * TODO: Extract real AccountTokenCertificate from device.
     * This is an X.509 certificate that signs the AccountToken.
     * Placeholder: empty data blob.
     */
    plist_dict_set_item(record, "AccountTokenCertificate",
                        plist_new_data("", 0));

    /*
     * TODO: Generate real AccountTokenSignature (ECDSA).
     * Signs the AccountToken with the certificate's private key.
     * Placeholder: empty data blob.
     */
    plist_dict_set_item(record, "AccountTokenSignature",
                        plist_new_data("", 0));

    /*
     * TODO: Extract DeviceCertificate from device identity.
     * This is the device's Apple-issued identity certificate.
     * Placeholder: empty data blob.
     */
    plist_dict_set_item(record, "DeviceCertificate",
                        plist_new_data("", 0));

    /*
     * TODO: Extract FairPlayKeyData from device.
     * Must match IC-Info.sisv exactly or activation will fail.
     * This is the FairPlay DRM key material.
     * Placeholder: empty data blob.
     */
    plist_dict_set_item(record, "FairPlayKeyData",
                        plist_new_data("", 0));

    /*
     * TODO: Extract UniqueDeviceCertificate from device.
     * Device-unique certificate used for activation binding.
     * Placeholder: empty data blob.
     */
    plist_dict_set_item(record, "UniqueDeviceCertificate",
                        plist_new_data("", 0));

    /* Protocol flags */
    plist_dict_set_item(record, "ack-received",
                        plist_new_bool(1));
    plist_dict_set_item(record, "unbrick",
                        plist_new_bool(1));

    return 0;
}

/* --- Public API --- */

plist_t record_build(const device_info_t *dev)
{
    plist_t record = NULL;

    if (!dev) {
        log_error("%s NULL device info", LOG_TAG);
        return NULL;
    }

    record = plist_new_dict();
    if (!record) {
        log_error("%s Failed to allocate plist dict", LOG_TAG);
        return NULL;
    }

    if (populate_common_fields(record, dev) < 0) {
        plist_free(record);
        return NULL;
    }

    log_info("%s Built standard activation record for %s (%s)",
             LOG_TAG, dev->udid, dev->product_type);

    return record;
}

plist_t record_build_a12(const device_info_t *dev)
{
    plist_t record = NULL;
    char ecid_str[32];

    if (!dev) {
        log_error("%s NULL device info", LOG_TAG);
        return NULL;
    }

    record = plist_new_dict();
    if (!record) {
        log_error("%s Failed to allocate plist dict", LOG_TAG);
        return NULL;
    }

    if (populate_common_fields(record, dev) < 0) {
        plist_free(record);
        return NULL;
    }

    /*
     * A12+ additional fields for SEP-aware activation.
     * The drmHandshake/FActivation path requires these.
     */

    /* ECID: exclusive chip identifier (uint64 stored as string) */
    snprintf(ecid_str, sizeof(ecid_str), "%llu",
             (unsigned long long)dev->ecid);
    plist_dict_set_item(record, "ECID",
                        plist_new_string(ecid_str));

    /* Hardware model for A12+ SEP binding */
    plist_dict_set_item(record, "HardwareModel",
                        plist_new_string(dev->hardware_model));

    /* Chip ID as integer for version-specific handling */
    plist_dict_set_item(record, "ChipID",
                        plist_new_uint(dev->cpid));

    /*
     * TODO: Include RKC (Remote Key Chain) data for A12+ SEP.
     * Required for Secure Enclave activation state binding.
     * Placeholder: empty data blob.
     */
    plist_dict_set_item(record, "RKCData",
                        plist_new_data("", 0));

    /*
     * TODO: Include SepNonce from device for replay protection.
     * Placeholder: empty data blob.
     */
    plist_dict_set_item(record, "SepNonce",
                        plist_new_data("", 0));

    /* Mark as session-mode activation (drmHandshake path) */
    plist_dict_set_item(record, "SessionMode",
                        plist_new_bool(1));

    log_info("%s Built A12+ activation record for %s (ECID %s, chip 0x%04x)",
             LOG_TAG, dev->udid, ecid_str, dev->cpid);

    return record;
}

char *record_to_xml(plist_t record, uint32_t *len)
{
    char *xml = NULL;

    if (!record || !len) {
        log_error("%s Invalid arguments to record_to_xml", LOG_TAG);
        return NULL;
    }

    plist_to_xml(record, &xml, len);
    if (!xml || *len == 0) {
        log_error("%s plist_to_xml failed", LOG_TAG);
        return NULL;
    }

    return xml;
}

void record_free(plist_t record)
{
    if (record)
        plist_free(record);
}
