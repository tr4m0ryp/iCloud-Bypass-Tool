/*
 * test_signer.c -- ECDSA P-256 signer tests (Phase 2A).
 *
 * Generates a real EC keypair on disk, then signs and verifies
 * synthetic payloads through the signer_* API. Signer pulls in
 * libcrypto, which is only linked under `make test-mocks`; the plain
 * `make test` build therefore prints a skip banner.
 */

#include "test_framework.h"

#ifdef TEST_MODE

#include "activation/signer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Per-test fixture                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    char priv[128];
    char pub[128];
} signer_tmp_t;

static void signer_tmp_init(signer_tmp_t *tf, const char *tag)
{
    int pid = (int)getpid();
    snprintf(tf->priv, sizeof(tf->priv),
             "/tmp/tr4mpass_test_%d_%s_priv.pem", pid, tag);
    snprintf(tf->pub, sizeof(tf->pub),
             "/tmp/tr4mpass_test_%d_%s_pub.pem", pid, tag);
    unlink(tf->priv);
    unlink(tf->pub);
}

static void signer_tmp_cleanup(signer_tmp_t *tf)
{
    unlink(tf->priv);
    unlink(tf->pub);
}

static int file_exists_nonempty(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_size > 0 ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* Cases                                                              */
/* ------------------------------------------------------------------ */

static void test_generate_keypair_creates_files(void)
{
    signer_tmp_t tf;
    signer_tmp_init(&tf, "gen");

    ASSERT_EQ(signer_generate_test_keypair(tf.priv, tf.pub), 0);
    ASSERT_EQ(file_exists_nonempty(tf.priv), 1);
    ASSERT_EQ(file_exists_nonempty(tf.pub),  1);

    /* Sanity: private PEM should start with -----BEGIN. */
    {
        FILE *f = fopen(tf.priv, "r");
        char  header[32] = {0};
        ASSERT_NOTNULL(f);
        if (f) {
            size_t n = fread(header, 1, sizeof(header) - 1, f);
            fclose(f);
            ASSERT_GT(n, (size_t)0);
            ASSERT_STRSTR(header, "BEGIN");
        }
    }

    signer_tmp_cleanup(&tf);
}

static void test_sign_and_verify_roundtrip(void)
{
    signer_tmp_t    tf;
    unsigned char  *sig = NULL;
    size_t          sig_len = 0;
    const unsigned char payload[] = "the quick brown fox jumps over lazy dog";

    signer_tmp_init(&tf, "rtrip");
    ASSERT_EQ(signer_generate_test_keypair(tf.priv, tf.pub), 0);

    ASSERT_EQ(signer_ecdsa_sign(tf.priv,
                                payload, sizeof(payload) - 1,
                                &sig, &sig_len), 0);
    ASSERT_NOTNULL(sig);
    ASSERT_GT(sig_len, (size_t)0);
    /* DER P-256 signatures are roughly 70-72 bytes. */
    ASSERT_LT(sig_len, (size_t)128);

    ASSERT_EQ(signer_ecdsa_verify(tf.pub,
                                  payload, sizeof(payload) - 1,
                                  sig, sig_len), 1);

    free(sig);
    signer_tmp_cleanup(&tf);
}

static void test_verify_rejects_tampered_data(void)
{
    signer_tmp_t   tf;
    unsigned char *sig = NULL;
    size_t         sig_len = 0;
    unsigned char  payload[32];

    signer_tmp_init(&tf, "tdata");
    memset(payload, 0x41, sizeof(payload));

    ASSERT_EQ(signer_generate_test_keypair(tf.priv, tf.pub), 0);
    ASSERT_EQ(signer_ecdsa_sign(tf.priv, payload, sizeof(payload),
                                &sig, &sig_len), 0);

    /* Flip one byte of the payload. */
    payload[7] ^= 0x01;
    ASSERT_EQ(signer_ecdsa_verify(tf.pub, payload, sizeof(payload),
                                  sig, sig_len), 0);

    free(sig);
    signer_tmp_cleanup(&tf);
}

static void test_verify_rejects_tampered_sig(void)
{
    signer_tmp_t   tf;
    unsigned char *sig = NULL;
    size_t         sig_len = 0;
    unsigned char  payload[32];

    signer_tmp_init(&tf, "tsig");
    memset(payload, 0x5A, sizeof(payload));

    ASSERT_EQ(signer_generate_test_keypair(tf.priv, tf.pub), 0);
    ASSERT_EQ(signer_ecdsa_sign(tf.priv, payload, sizeof(payload),
                                &sig, &sig_len), 0);
    ASSERT_GT(sig_len, (size_t)4);

    /* Flip one byte in the middle of the DER signature. */
    sig[sig_len / 2] ^= 0xFF;
    {
        int v = signer_ecdsa_verify(tf.pub, payload, sizeof(payload),
                                    sig, sig_len);
        /* OpenSSL may report 0 (bad sig) or -1 (malformed DER); both
         * mean the tampered signature is rejected. */
        ASSERT(v != 1);
    }

    free(sig);
    signer_tmp_cleanup(&tf);
}

static void test_sign_missing_key_file(void)
{
    unsigned char *sig = NULL;
    size_t         sig_len = 0;
    const unsigned char payload[] = "hello";

    ASSERT_EQ(signer_ecdsa_sign("/tmp/tr4mpass_no_such_key.pem",
                                payload, sizeof(payload) - 1,
                                &sig, &sig_len), -1);
    ASSERT_NULL(sig);
    ASSERT_EQ(sig_len, (size_t)0);
}

static void test_sign_empty_data(void)
{
    signer_tmp_t   tf;
    unsigned char *sig = NULL;
    size_t         sig_len = 0;

    signer_tmp_init(&tf, "empty");
    ASSERT_EQ(signer_generate_test_keypair(tf.priv, tf.pub), 0);

    /* OpenSSL accepts empty input: SHA-256 of empty is well-defined. */
    ASSERT_EQ(signer_ecdsa_sign(tf.priv, NULL, 0, &sig, &sig_len), 0);
    ASSERT_NOTNULL(sig);
    ASSERT_GT(sig_len, (size_t)0);

    ASSERT_EQ(signer_ecdsa_verify(tf.pub, NULL, 0, sig, sig_len), 1);

    free(sig);
    signer_tmp_cleanup(&tf);
}

#endif /* TEST_MODE */

void run_signer_tests(void)
{
    printf("--- Section 6: ECDSA signer ---\n");
#ifdef TEST_MODE
    test_generate_keypair_creates_files();
    test_sign_and_verify_roundtrip();
    test_verify_rejects_tampered_data();
    test_verify_rejects_tampered_sig();
    test_sign_missing_key_file();
    test_sign_empty_data();
#else
    printf("  (skipped: requires mock build, run `make test-mocks`)\n");
#endif
}
