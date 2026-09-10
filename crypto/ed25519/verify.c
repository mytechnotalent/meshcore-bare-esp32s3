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
 * verify.c
 *
 * Ed25519 digital signature verification.
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
 * @brief Compare two 32-byte arrays in constant time.
 *
 * @param x Pointer to first 32-byte array.
 * @param y Pointer to second 32-byte array.
 * @return int Returns 1 if arrays are equal, 0 otherwise.
 */
static int consttime_equal(const unsigned char *x, const unsigned char *y) {
    unsigned char r = 0;
    r = x[0] ^ y[0];
#define F(i) r |= x[i] ^ y[i]
    F(1);
    F(2);
    F(3);
    F(4);
    F(5);
    F(6);
    F(7);
    F(8);
    F(9);
    F(10);
    F(11);
    F(12);
    F(13);
    F(14);
    F(15);
    F(16);
    F(17);
    F(18);
    F(19);
    F(20);
    F(21);
    F(22);
    F(23);
    F(24);
    F(25);
    F(26);
    F(27);
    F(28);
    F(29);
    F(30);
    F(31);
#undef F
    return !r;
}

/**
 * @brief Verify an Ed25519 signature against a message and public key.
 *
 * @param signature Pointer to 64-byte signature.
 * @param message Pointer to message buffer.
 * @param message_len Length of message in bytes.
 * @param public_key Pointer to 32-byte public key.
 * @return int Returns 1 if valid, 0 if invalid.
 */
int ed25519_verify(const unsigned char *signature,
                   const unsigned char *message,
                   size_t message_len,
                   const unsigned char *public_key) {
    unsigned char h[64];
    unsigned char checker[32];
    sha512_context hash;
    ge_p3 A;
    ge_p2 R;
    if (signature[63] & 224) {
        return 0;
    }
    if (ge_frombytes_negate_vartime(&A, public_key) != 0) {
        return 0;
    }
    sha512_init(&hash);
    sha512_update(&hash, signature, 32);
    sha512_update(&hash, public_key, 32);
    sha512_update(&hash, message, message_len);
    sha512_final(&hash, h);
    sc_reduce(h);
    ge_double_scalarmult_vartime(&R, h, &A, signature + 32);
    ge_tobytes(checker, &R);
    if (!consttime_equal(checker, signature)) {
        return 0;
    }
    return 1;
}
