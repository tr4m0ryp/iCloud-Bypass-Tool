/*
 * cert_extract.h -- Device certificate extraction via lockdownd.
 *
 * Pulls the three certificate blobs that the activation record needs
 * directly from a connected device's lockdownd service:
 *
 *   - DeviceCertificate
 *   - AccountTokenCertificate
 *   - UniqueDeviceCertificate
 *
 * Each is returned as a raw byte blob (DER-encoded X.509 in practice).
 * The caller owns the buffer and must release it with cert_blob_free().
 */

#ifndef CERT_EXTRACT_H
#define CERT_EXTRACT_H

#include <stddef.h>

#include <libimobiledevice/lockdown.h>

typedef struct {
    unsigned char *data;
    size_t         len;
} cert_blob_t;

/*
 * cert_blob_free -- Release a cert_blob_t's backing buffer.
 *
 * Safe to call on a NULL pointer or a zeroed blob. After the call,
 * blob->data is NULL and blob->len is zero.
 */
void cert_blob_free(cert_blob_t *blob);

/*
 * cert_extract_device_certificate -- Read the DeviceCertificate.
 *
 * Queries the key "DeviceCertificate" in domain
 * "com.apple.mobile.lockdown". On success returns 0 and populates *out;
 * on any failure returns -1 and leaves *out zeroed.
 */
int cert_extract_device_certificate(lockdownd_client_t client,
                                    cert_blob_t *out);

/*
 * cert_extract_account_token_certificate -- Read the AccountTokenCertificate.
 *
 * Tries, in order:
 *   1. key "AccountTokenCertificate" (NULL domain)
 *   2. key "ActivationStateDeadline" (NULL domain)
 *   3. first element of "CertificateChain" array (NULL domain)
 *
 * Returns 0 on success and populates *out; -1 on failure.
 */
int cert_extract_account_token_certificate(lockdownd_client_t client,
                                           cert_blob_t *out);

/*
 * cert_extract_unique_device_certificate -- Read the UniqueDeviceCertificate.
 *
 * Queries the key "UniqueDeviceCertificate" in domain
 * "com.apple.mobile.activation". Returns 0 on success, -1 on failure.
 */
int cert_extract_unique_device_certificate(lockdownd_client_t client,
                                           cert_blob_t *out);

#endif /* CERT_EXTRACT_H */
