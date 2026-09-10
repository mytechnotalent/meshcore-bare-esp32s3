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
 * keypair.c
 *
 * Ed25519 keypair generation and public key derivation.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#include "ed_25519.h"
#include "ge.h"
#include "sha512.h"

/**
 * @brief Create an Ed25519 public and private key pair from a seed.
 *
 * @param public_key Pointer to output buffer for 32-byte public key.
 * @param private_key Pointer to output buffer for 64-byte private key.
 * @param seed Pointer to 32-byte seed.
 * @return void
 */
void ed25519_create_keypair(unsigned char *public_key,
                            unsigned char *private_key,
                            const unsigned char *seed) {
    ge_p3 A;
    sha512(seed, 32, private_key);
    private_key[0] &= 248;
    private_key[31] &= 63;
    private_key[31] |= 64;
    ge_scalarmult_base(&A, private_key);
    ge_p3_tobytes(public_key, &A);
}

/**
 * @brief Derive an Ed25519 public key from a private key.
 *
 * @param public_key Pointer to output buffer for 32-byte public key.
 * @param private_key Pointer to 64-byte private key.
 * @return void
 */
void ed25519_derive_pub(unsigned char *public_key, const unsigned char *private_key) {
    ge_p3 A;
    ge_scalarmult_base(&A, private_key);
    ge_p3_tobytes(public_key, &A);
}
