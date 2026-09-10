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
 * fe.h
 *
 * Field element arithmetic for Ed25519 in Z/(2^255 - 19).
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#ifndef FE_H
#define FE_H

#include "fixedint.h"

/**
 * @brief Field element represented as 10 32-bit signed integers.
 *
 * An element t, entries t[0]...t[9], represents the integer
 * t[0] + 2^26 t[1] + 2^51 t[2] + 2^77 t[3] + 2^102 t[4] + ... + 2^230 t[9].
 */
typedef int32_t fe[10];

/**
 * @brief Set field element to zero.
 *
 * @param h Field element to set to zero.
 * @return void
 */
void fe_0(fe h);

/**
 * @brief Set field element to one.
 *
 * @param h Field element to set to one.
 * @return void
 */
void fe_1(fe h);

/**
 * @brief Decode 32-byte array to field element.
 *
 * @param h Output field element.
 * @param s 32-byte input array.
 * @return void
 */
void fe_frombytes(fe h, const unsigned char *s);

/**
 * @brief Encode field element to 32-byte array.
 *
 * @param s 32-byte output array.
 * @param h Input field element.
 * @return void
 */
void fe_tobytes(unsigned char *s, const fe h);

/**
 * @brief Copy field element f to h.
 *
 * @param h Destination field element.
 * @param f Source field element.
 * @return void
 */
void fe_copy(fe h, const fe f);

/**
 * @brief Check if field element is negative.
 *
 * @param f Input field element.
 * @return int 1 if negative, 0 otherwise.
 */
int fe_isnegative(const fe f);

/**
 * @brief Check if field element is non-zero.
 *
 * @param f Input field element.
 * @return int 1 if non-zero, 0 if zero.
 */
int fe_isnonzero(const fe f);

/**
 * @brief Conditional move: f = g if b != 0.
 *
 * @param f Destination field element.
 * @param g Source field element.
 * @param b Condition flag (0 or 1).
 * @return void
 */
void fe_cmov(fe f, const fe g, unsigned int b);

/**
 * @brief Conditional swap: swap f and g if b != 0.
 *
 * @param f First field element.
 * @param g Second field element.
 * @param b Condition flag (0 or 1).
 * @return void
 */
void fe_cswap(fe f, fe g, unsigned int b);

/**
 * @brief Negate field element: h = -f.
 *
 * @param h Output field element.
 * @param f Input field element.
 * @return void
 */
void fe_neg(fe h, const fe f);

/**
 * @brief Add field elements: h = f + g.
 *
 * @param h Output field element.
 * @param f First operand.
 * @param g Second operand.
 * @return void
 */
void fe_add(fe h, const fe f, const fe g);

/**
 * @brief Invert field element modulo 2^255 - 19: out = 1 / z.
 *
 * @param out Output field element.
 * @param z Input field element.
 * @return void
 */
void fe_invert(fe out, const fe z);

/**
 * @brief Square field element: h = f^2.
 *
 * @param h Output field element.
 * @param f Input field element.
 * @return void
 */
void fe_sq(fe h, const fe f);

/**
 * @brief Square field element and double: h = 2 * f^2.
 *
 * @param h Output field element.
 * @param f Input field element.
 * @return void
 */
void fe_sq2(fe h, const fe f);

/**
 * @brief Multiply field elements: h = f * g.
 *
 * @param h Output field element.
 * @param f First operand.
 * @param g Second operand.
 * @return void
 */
void fe_mul(fe h, const fe f, const fe g);

/**
 * @brief Multiply field element by curve constant 121666: h = 121666 * f.
 *
 * @param h Output field element.
 * @param f Input field element.
 * @return void
 */
void fe_mul121666(fe h, fe f);

/**
 * @brief Compute z^(2^252 - 3) modulo 2^255 - 19.
 *
 * @param out Output field element.
 * @param z Input field element.
 * @return void
 */
void fe_pow22523(fe out, const fe z);

/**
 * @brief Subtract field elements: h = f - g.
 *
 * @param h Output field element.
 * @param f First operand.
 * @param g Second operand.
 * @return void
 */
void fe_sub(fe h, const fe f, const fe g);

#endif // FE_H
