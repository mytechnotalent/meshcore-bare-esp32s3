/**
 * MIT License
 *
 * Copyright (c) 2026 Kevin Thomas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ed_25519.h
 *
 * Ed25519 public key signature system declarations.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#ifndef ED25519_H
#define ED25519_H

#include <stddef.h>

#if defined(_WIN32)
#if defined(ED25519_BUILD_DLL)
#define ED25519_DECLSPEC __declspec(dllexport)
#elif defined(ED25519_DLL)
#define ED25519_DECLSPEC __declspec(dllimport)
#else
#define ED25519_DECLSPEC
#endif
#else
#define ED25519_DECLSPEC
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ED25519_NO_SEED
/**
 * @brief Generate a cryptographically secure 32-byte seed.
 *
 * @param seed Pointer to output buffer for the 32-byte seed.
 * @return int Returns 0 on success, non-zero on failure.
 */
int ED25519_DECLSPEC ed25519_create_seed(unsigned char *seed);
#endif

/**
 * @brief Create an Ed25519 public and private key pair from a seed.
 *
 * @param public_key Pointer to output buffer for 32-byte public key.
 * @param private_key Pointer to output buffer for 64-byte private key.
 * @param seed Pointer to 32-byte seed.
 * @return void
 */
void ED25519_DECLSPEC ed25519_create_keypair(unsigned char *public_key,
                                             unsigned char *private_key,
                                             const unsigned char *seed);

/**
 * @brief Derive an Ed25519 public key from a private key.
 *
 * @param public_key Pointer to output buffer for 32-byte public key.
 * @param private_key Pointer to 64-byte private key.
 * @return void
 */
void ED25519_DECLSPEC ed25519_derive_pub(unsigned char *public_key,
                                         const unsigned char *private_key);

/**
 * @brief Sign a message using an Ed25519 private key.
 *
 * @param signature Pointer to output buffer for 64-byte signature.
 * @param message Pointer to message buffer.
 * @param message_len Length of message in bytes.
 * @param public_key Pointer to 32-byte public key.
 * @param private_key Pointer to 64-byte private key.
 * @return void
 */
void ED25519_DECLSPEC ed25519_sign(unsigned char *signature,
                                   const unsigned char *message,
                                   size_t message_len,
                                   const unsigned char *public_key,
                                   const unsigned char *private_key);

/**
 * @brief Verify an Ed25519 signature against a message and public key.
 *
 * @param signature Pointer to 64-byte signature.
 * @param message Pointer to message buffer.
 * @param message_len Length of message in bytes.
 * @param public_key Pointer to 32-byte public key.
 * @return int Returns 1 if valid, 0 if invalid.
 */
int ED25519_DECLSPEC ed25519_verify(const unsigned char *signature,
                                    const unsigned char *message,
                                    size_t message_len,
                                    const unsigned char *public_key);

/**
 * @brief Add a scalar to both public and private keys.
 *
 * @param public_key Pointer to 32-byte public key to be updated.
 * @param private_key Pointer to 64-byte private key to be updated.
 * @param scalar Pointer to 32-byte scalar.
 * @return void
 */
void ED25519_DECLSPEC ed25519_add_scalar(unsigned char *public_key,
                                         unsigned char *private_key,
                                         const unsigned char *scalar);

/**
 * @brief Perform Diffie-Hellman key exchange on Curve25519.
 *
 * @param shared_secret Pointer to output buffer for 32-byte shared secret.
 * @param public_key Pointer to 32-byte public key.
 * @param private_key Pointer to 64-byte private key.
 * @return void
 */
void ED25519_DECLSPEC ed25519_key_exchange(unsigned char *shared_secret,
                                           const unsigned char *public_key,
                                           const unsigned char *private_key);

#ifdef __cplusplus
}
#endif

#endif // ED25519_H
