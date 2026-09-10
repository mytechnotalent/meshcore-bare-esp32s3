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
 * sign.c
 *
 * Ed25519 digital signature generation.
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
 * @brief Sign a message using an Ed25519 private key.
 *
 * @param signature Pointer to output buffer for 64-byte signature.
 * @param message Pointer to message buffer.
 * @param message_len Length of message in bytes.
 * @param public_key Pointer to 32-byte public key.
 * @param private_key Pointer to 64-byte private key.
 * @return void
 */
void ed25519_sign(unsigned char *signature,
                  const unsigned char *message,
                  size_t message_len,
                  const unsigned char *public_key,
                  const unsigned char *private_key) {
    sha512_context hash;
    unsigned char hram[64];
    unsigned char r[64];
    ge_p3 R;
    sha512_init(&hash);
    sha512_update(&hash, private_key + 32, 32);
    sha512_update(&hash, message, message_len);
    sha512_final(&hash, r);
    sc_reduce(r);
    ge_scalarmult_base(&R, r);
    ge_p3_tobytes(signature, &R);
    sha512_init(&hash);
    sha512_update(&hash, signature, 32);
    sha512_update(&hash, public_key, 32);
    sha512_update(&hash, message, message_len);
    sha512_final(&hash, hram);
    sc_reduce(hram);
    sc_muladd(signature + 32, hram, private_key, r);
}
