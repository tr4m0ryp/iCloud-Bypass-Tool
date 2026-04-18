/*
 * signer.h -- ECDSA P-256 signing and verification helpers.
 *
 * Thin wrapper over OpenSSL 3.x EVP API. All signatures are ECDSA over
 * SHA-256, DER-encoded (not raw r||s). Keys are read from and written
 * to PEM files on disk.
 */

#ifndef SIGNER_H
#define SIGNER_H

#include <stddef.h>

/*
 * signer_ecdsa_sign -- Sign a byte buffer with an EC private key on disk.
 *
 * Reads a P-256 private key from `pem_path`, signs `data[len]` (SHA-256
 * first), and returns the DER-encoded signature via *sig / *sig_len.
 *
 * The output buffer is malloc'd and the caller must free() it.
 * Returns 0 on success, -1 on any failure.
 */
int signer_ecdsa_sign(const char *pem_path,
                      const unsigned char *data, size_t len,
                      unsigned char **sig, size_t *sig_len);

/*
 * signer_ecdsa_verify -- Verify a DER-encoded ECDSA signature.
 *
 * Returns 1 if `sig[sig_len]` is a valid signature over SHA-256 of
 * `data[len]` under the public key in `pem_path`. Returns 0 if the
 * signature is well-formed but invalid, and -1 on any system error
 * (bad file, malformed PEM, OOM, etc.).
 */
int signer_ecdsa_verify(const char *pem_path,
                        const unsigned char *data, size_t len,
                        const unsigned char *sig, size_t sig_len);

/*
 * signer_generate_test_keypair -- Write a fresh P-256 keypair to disk.
 *
 * Intended for tests. Produces two files: an unencrypted PKCS#8 private
 * PEM and a SubjectPublicKeyInfo PEM. Overwrites existing files.
 * Returns 0 on success, -1 on failure.
 */
int signer_generate_test_keypair(const char *priv_pem_path,
                                 const char *pub_pem_path);

#endif /* SIGNER_H */
