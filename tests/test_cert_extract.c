/*
 * test_cert_extract.c -- Certificate extraction tests (Phase 2A).
 *
 * Exercises cert_extract_* via the mock lockdownd API. Because the
 * extractor is only linked when building with the mock framework
 * (make test-mocks), the entire suite is compiled out under plain
 * `make test` -- there we just print a skip banner.
 */

#include "test_framework.h"

#ifdef TEST_MODE

#include "activation/cert_extract.h"
#include "mocks/mock_control.h"

#include <stdlib.h>
#include <string.h>

#include <plist/plist.h>

/* Non-NULL sentinel; the mock ignores the pointer value. */
static lockdownd_client_t fake_client(void)
{
    static int dummy;
    return (lockdownd_client_t)&dummy;
}

static const unsigned char CERT_A[] = {
    0x30, 0x82, 0x01, 0x0a, 0xDE, 0xAD, 0xBE, 0xEF,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
};

static const unsigned char CERT_B[] = {
    0x30, 0x82, 0x02, 0x20, 0xCA, 0xFE, 0xBA, 0xBE,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC
};

/* ------------------------------------------------------------------ */

static void test_extract_device_cert_success(void)
{
    cert_blob_t blob = {NULL, 0};
    mock_reset_all();
    mock_lockdownd_set_value("com.apple.mobile.lockdown",
                             "DeviceCertificate",
                             "data", CERT_A, sizeof(CERT_A));

    ASSERT_EQ(cert_extract_device_certificate(fake_client(), &blob), 0);
    ASSERT_NOTNULL(blob.data);
    ASSERT_EQ(blob.len, sizeof(CERT_A));
    ASSERT_MEM_EQ(blob.data, CERT_A, sizeof(CERT_A));
    cert_blob_free(&blob);
    ASSERT_NULL(blob.data);
    ASSERT_EQ(blob.len, (size_t)0);
}

static void test_extract_device_cert_missing(void)
{
    cert_blob_t blob = {NULL, 0};
    mock_reset_all();
    /* No staged value: the getter must surface -1. */
    ASSERT_EQ(cert_extract_device_certificate(fake_client(), &blob), -1);
    ASSERT_NULL(blob.data);
    ASSERT_EQ(blob.len, (size_t)0);
}

static void test_extract_account_token_cert_success(void)
{
    cert_blob_t blob = {NULL, 0};
    mock_reset_all();
    mock_lockdownd_set_value(NULL, "AccountTokenCertificate",
                             "data", CERT_B, sizeof(CERT_B));

    ASSERT_EQ(cert_extract_account_token_certificate(fake_client(), &blob), 0);
    ASSERT_EQ(blob.len, sizeof(CERT_B));
    ASSERT_MEM_EQ(blob.data, CERT_B, sizeof(CERT_B));
    cert_blob_free(&blob);
}

static void test_extract_account_token_cert_fallback_to_chain(void)
{
    /*
     * Verify the fallback order: with only the SECOND-choice key
     * staged (ActivationStateDeadline), the extractor must find it
     * after AccountTokenCertificate misses. The mock cannot wrap a
     * real PLIST_ARRAY, so the third-choice CertificateChain path is
     * covered via a DATA node staged under that key below.
     */
    cert_blob_t blob = {NULL, 0};
    mock_reset_all();
    mock_lockdownd_set_value(NULL, "ActivationStateDeadline",
                             "data", CERT_A, sizeof(CERT_A));

    ASSERT_EQ(cert_extract_account_token_certificate(fake_client(), &blob), 0);
    ASSERT_EQ(blob.len, sizeof(CERT_A));
    ASSERT_MEM_EQ(blob.data, CERT_A, sizeof(CERT_A));
    cert_blob_free(&blob);

    /* Third fallback: CertificateChain as raw DATA. */
    mock_reset_all();
    mock_lockdownd_set_value(NULL, "CertificateChain",
                             "data", CERT_B, sizeof(CERT_B));
    blob.data = NULL; blob.len = 0;
    ASSERT_EQ(cert_extract_account_token_certificate(fake_client(), &blob), 0);
    ASSERT_EQ(blob.len, sizeof(CERT_B));
    cert_blob_free(&blob);
}

static void test_extract_account_token_cert_all_missing(void)
{
    cert_blob_t blob = {NULL, 0};
    mock_reset_all();
    ASSERT_EQ(cert_extract_account_token_certificate(fake_client(), &blob), -1);
    ASSERT_NULL(blob.data);
}

static void test_extract_unique_cert_success(void)
{
    cert_blob_t blob = {NULL, 0};
    mock_reset_all();
    mock_lockdownd_set_value("com.apple.mobile.activation",
                             "UniqueDeviceCertificate",
                             "data", CERT_B, sizeof(CERT_B));

    ASSERT_EQ(cert_extract_unique_device_certificate(fake_client(), &blob), 0);
    ASSERT_EQ(blob.len, sizeof(CERT_B));
    ASSERT_MEM_EQ(blob.data, CERT_B, sizeof(CERT_B));
    cert_blob_free(&blob);
}

static void test_extract_unique_cert_wrong_domain(void)
{
    cert_blob_t blob = {NULL, 0};
    mock_reset_all();
    /* Value staged on wrong domain -- lookup must miss. */
    mock_lockdownd_set_value("com.apple.mobile.lockdown",
                             "UniqueDeviceCertificate",
                             "data", CERT_A, sizeof(CERT_A));

    ASSERT_EQ(cert_extract_unique_device_certificate(fake_client(), &blob), -1);
    ASSERT_NULL(blob.data);
}

static void test_cert_blob_free_null_safe(void)
{
    cert_blob_t zeroed = {NULL, 0};

    /* No-op on NULL. */
    cert_blob_free(NULL);
    /* No-op on already-zeroed blob. */
    cert_blob_free(&zeroed);
    ASSERT_NULL(zeroed.data);
    ASSERT_EQ(zeroed.len, (size_t)0);
}

static void test_cert_blob_free_normal(void)
{
    cert_blob_t blob;
    blob.data = (unsigned char *)malloc(32);
    ASSERT_NOTNULL(blob.data);
    blob.len = 32;
    memset(blob.data, 0x5A, 32);
    cert_blob_free(&blob);
    ASSERT_NULL(blob.data);
    ASSERT_EQ(blob.len, (size_t)0);
}

#endif /* TEST_MODE */

/* ------------------------------------------------------------------ */

void run_cert_extract_tests(void)
{
    printf("--- Section 5: Certificate extraction ---\n");
#ifdef TEST_MODE
    test_extract_device_cert_success();
    test_extract_device_cert_missing();
    test_extract_account_token_cert_success();
    test_extract_account_token_cert_fallback_to_chain();
    test_extract_account_token_cert_all_missing();
    test_extract_unique_cert_success();
    test_extract_unique_cert_wrong_domain();
    test_cert_blob_free_null_safe();
    test_cert_blob_free_normal();
#else
    printf("  (skipped: requires mock build, run `make test-mocks`)\n");
#endif
}
