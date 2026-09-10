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
 * add_scalar.c
 *
 * Scalar addition for Ed25519 keypairs.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#include "ed_25519.h"
#include "ge.h"
#include "sc.h"
#include "sha512.h"

/**
 * @brief Add a scalar to both public and private keys.
 *
 * @param public_key Pointer to 32-byte public key to be updated.
 * @param private_key Pointer to 64-byte private key to be updated.
 * @param scalar Pointer to 32-byte scalar.
 * @return void
 */
void ed25519_add_scalar(unsigned char *public_key,
                        unsigned char *private_key,
                        const unsigned char *scalar) {
    const unsigned char SC_1[32] = {1};
    unsigned char n[32];
    ge_p3 nB;
    ge_p1p1 A_p1p1;
    ge_p3 A;
    ge_p3 public_key_unpacked;
    ge_cached T;
    sha512_context hash;
    unsigned char hashbuf[64];
    int i;
    for (i = 0; i < 31; ++i) {
        n[i] = scalar[i];
    }
    n[31] = scalar[31] & 127;
    if (private_key) {
        sc_muladd(private_key, SC_1, n, private_key);
        sha512_init(&hash);
        sha512_update(&hash, private_key + 32, 32);
        sha512_update(&hash, scalar, 32);
        sha512_final(&hash, hashbuf);
        for (i = 0; i < 32; ++i) {
            private_key[32 + i] = hashbuf[i];
        }
    }
    if (public_key) {
        if (private_key) {
            ge_scalarmult_base(&A, private_key);
        } else {
            ge_frombytes_negate_vartime(&public_key_unpacked, public_key);
            fe_neg(public_key_unpacked.X, public_key_unpacked.X);
            fe_neg(public_key_unpacked.T, public_key_unpacked.T);
            ge_p3_to_cached(&T, &public_key_unpacked);
            ge_scalarmult_base(&nB, n);
            ge_add(&A_p1p1, &nB, &T);
            ge_p1p1_to_p3(&A, &A_p1p1);
        }
        ge_p3_tobytes(public_key, &A);
    }
}
