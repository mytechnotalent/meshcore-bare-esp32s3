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
 * sha512.h
 *
 * SHA-512 cryptographic hash function declarations.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#ifndef SHA512_H
#define SHA512_H

#include "fixedint.h"

#include <stddef.h>

/**
 * @brief SHA-512 context state structure.
 */
typedef struct sha512_context_ {
    /**
     * @brief Total length of hashed message in bits.
     */
    uint64_t length;
    /**
     * @brief 512-bit hash state array of eight 64-bit words.
     */
    uint64_t state[8];
    /**
     * @brief Current length of pending data in the block buffer.
     */
    size_t curlen;
    /**
     * @brief 128-byte input block buffer.
     */
    unsigned char buf[128];
} sha512_context;

/**
 * @brief Initialize a SHA-512 context.
 *
 * @param md Pointer to the SHA-512 context.
 * @return int Returns 0 on success, non-zero on error.
 */
int sha512_init(sha512_context *md);

/**
 * @brief Finalize the SHA-512 context and extract the 64-byte digest.
 *
 * @param md Pointer to the SHA-512 context.
 * @param out Output buffer for the 64-byte hash digest.
 * @return int Returns 0 on success, non-zero on error.
 */
int sha512_final(sha512_context *md, unsigned char *out);

/**
 * @brief Feed data into an active SHA-512 context.
 *
 * @param md Pointer to the SHA-512 context.
 * @param in Pointer to input data to hash.
 * @param inlen Length of input data in bytes.
 * @return int Returns 0 on success, non-zero on error.
 */
int sha512_update(sha512_context *md, const unsigned char *in, size_t inlen);

/**
 * @brief Compute SHA-512 digest in a single call.
 *
 * @param message Pointer to message buffer.
 * @param message_len Length of message in bytes.
 * @param out Output buffer for 64-byte hash digest.
 * @return int Returns 0 on success, non-zero on error.
 */
int sha512(const unsigned char *message, size_t message_len, unsigned char *out);

#endif // SHA512_H
