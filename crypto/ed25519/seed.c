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
 * seed.c
 *
 * Cryptographically secure random seed generation for Ed25519.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#include "ed_25519.h"

#ifndef ED25519_NO_SEED

#ifdef _WIN32
#include <wincrypt.h>
#include <windows.h>
#else
#include <stdio.h>
#endif

/**
 * @brief Generate a cryptographically secure 32-byte seed.
 *
 * @param seed Pointer to output buffer for the 32-byte seed.
 * @return int Returns 0 on success, non-zero on failure.
 */
int ed25519_create_seed(unsigned char *seed) {
#ifdef _WIN32
    HCRYPTPROV prov;
    if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return 1;
    }
    if (!CryptGenRandom(prov, 32, seed)) {
        CryptReleaseContext(prov, 0);
        return 1;
    }
    CryptReleaseContext(prov, 0);
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (f == NULL) {
        return 1;
    }
    fread(seed, 1, 32, f);
    fclose(f);
#endif
    return 0;
}

#endif
