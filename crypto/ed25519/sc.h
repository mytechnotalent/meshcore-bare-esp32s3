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
 * sc.h
 *
 * Scalar arithmetic modulo group order l for Ed25519.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#ifndef SC_H
#define SC_H

/**
 * @brief Reduce 64-byte array modulo group order l: s = s mod l.
 *
 * The set of scalars is Z/l where
 * l = 2^252 + 27742317777372353535851937790883648493.
 *
 * @param s 64-byte array reduced in place to 32 bytes.
 * @return void
 */
void sc_reduce(unsigned char *s);

/**
 * @brief Compute s = (a * b + c) mod l for 32-byte scalars.
 *
 * @param s Output 32-byte scalar.
 * @param a First 32-byte scalar operand.
 * @param b Second 32-byte scalar operand.
 * @param c Third 32-byte scalar operand to add.
 * @return void
 */
void sc_muladd(unsigned char *s,
               const unsigned char *a,
               const unsigned char *b,
               const unsigned char *c);

#endif // SC_H
