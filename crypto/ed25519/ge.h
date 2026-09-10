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
 * ge.h
 *
 * Group element operations for Ed25519 elliptic curve arithmetic.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#ifndef GE_H
#define GE_H

#include "fe.h"

/**
 * @brief Group element in projective coordinates (X:Y:Z) where x=X/Z, y=Y/Z.
 */
typedef struct {
    /**
     * @brief Projective X coordinate.
     */
    fe X;
    /**
     * @brief Projective Y coordinate.
     */
    fe Y;
    /**
     * @brief Projective Z coordinate.
     */
    fe Z;
} ge_p2;

/**
 * @brief Group element in extended coordinates (X:Y:Z:T) where x=X/Z, y=Y/Z, XY=ZT.
 */
typedef struct {
    /**
     * @brief Extended X coordinate.
     */
    fe X;
    /**
     * @brief Extended Y coordinate.
     */
    fe Y;
    /**
     * @brief Extended Z coordinate.
     */
    fe Z;
    /**
     * @brief Extended T auxiliary coordinate (XY=ZT).
     */
    fe T;
} ge_p3;

/**
 * @brief Group element in completed coordinates ((X:Z),(Y:T)) where x=X/Z, y=Y/T.
 */
typedef struct {
    /**
     * @brief Completed X coordinate.
     */
    fe X;
    /**
     * @brief Completed Y coordinate.
     */
    fe Y;
    /**
     * @brief Completed Z coordinate.
     */
    fe Z;
    /**
     * @brief Completed T coordinate.
     */
    fe T;
} ge_p1p1;

/**
 * @brief Precomputed group element in Duif coordinates (y+x, y-x, 2dxy).
 */
typedef struct {
    /**
     * @brief Precomputed y + x.
     */
    fe yplusx;
    /**
     * @brief Precomputed y - x.
     */
    fe yminusx;
    /**
     * @brief Precomputed 2 * d * x * y.
     */
    fe xy2d;
} ge_precomp;

/**
 * @brief Cached group element for fast addition/subtraction.
 */
typedef struct {
    /**
     * @brief Cached Y + X.
     */
    fe YplusX;
    /**
     * @brief Cached Y - X.
     */
    fe YminusX;
    /**
     * @brief Cached Z coordinate.
     */
    fe Z;
    /**
     * @brief Cached 2 * d * T.
     */
    fe T2d;
} ge_cached;

/**
 * @brief Encode group element in extended coordinates (ge_p3) to 32 bytes.
 *
 * @param s 32-byte output buffer.
 * @param h Input group element in extended coordinates.
 * @return void
 */
void ge_p3_tobytes(unsigned char *s, const ge_p3 *h);

/**
 * @brief Encode group element in projective coordinates (ge_p2) to 32 bytes.
 *
 * @param s 32-byte output buffer.
 * @param h Input group element in projective coordinates.
 * @return void
 */
void ge_tobytes(unsigned char *s, const ge_p2 *h);

/**
 * @brief Decode 32-byte array to group element in extended coordinates with negated X.
 *
 * @param h Output group element in extended coordinates.
 * @param s 32-byte input buffer.
 * @return int 0 on success, -1 on invalid encoding.
 */
int ge_frombytes_negate_vartime(ge_p3 *h, const unsigned char *s);

/**
 * @brief Add group element in extended coordinates to cached group element: r = p + q.
 *
 * @param r Output completed group element.
 * @param p Extended group element operand.
 * @param q Cached group element operand.
 * @return void
 */
void ge_add(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q);

/**
 * @brief Subtract cached group element from extended group element: r = p - q.
 *
 * @param r Output completed group element.
 * @param p Extended group element operand.
 * @param q Cached group element operand.
 * @return void
 */
void ge_sub(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q);

/**
 * @brief Variable-time double scalar multiplication: r = a * A + b * B.
 *
 * @param r Output projective group element.
 * @param a 32-byte scalar multiplier for A.
 * @param A Extended group element.
 * @param b 32-byte scalar multiplier for basepoint B.
 * @return void
 */
void ge_double_scalarmult_vartime(ge_p2 *r,
                                  const unsigned char *a,
                                  const ge_p3 *A,
                                  const unsigned char *b);

/**
 * @brief Add precomputed group element to extended group element: r = p + q.
 *
 * @param r Output completed group element.
 * @param p Extended group element operand.
 * @param q Precomputed group element operand.
 * @return void
 */
void ge_madd(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q);

/**
 * @brief Subtract precomputed group element from extended group element: r = p - q.
 *
 * @param r Output completed group element.
 * @param p Extended group element operand.
 * @param q Precomputed group element operand.
 * @return void
 */
void ge_msub(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q);

/**
 * @brief Compute scalar multiplication with basepoint B: h = a * B.
 *
 * @param h Output extended group element.
 * @param a 32-byte scalar multiplier.
 * @return void
 */
void ge_scalarmult_base(ge_p3 *h, const unsigned char *a);

/**
 * @brief Convert completed group element (ge_p1p1) to projective coordinates (ge_p2).
 *
 * @param r Output projective group element.
 * @param p Input completed group element.
 * @return void
 */
void ge_p1p1_to_p2(ge_p2 *r, const ge_p1p1 *p);

/**
 * @brief Convert completed group element (ge_p1p1) to extended coordinates (ge_p3).
 *
 * @param r Output extended group element.
 * @param p Input completed group element.
 * @return void
 */
void ge_p1p1_to_p3(ge_p3 *r, const ge_p1p1 *p);

/**
 * @brief Set projective group element to neutral element (identity).
 *
 * @param h Output projective group element.
 * @return void
 */
void ge_p2_0(ge_p2 *h);

/**
 * @brief Double projective group element: r = 2 * p.
 *
 * @param r Output completed group element.
 * @param p Input projective group element.
 * @return void
 */
void ge_p2_dbl(ge_p1p1 *r, const ge_p2 *p);

/**
 * @brief Set extended group element to neutral element (identity).
 *
 * @param h Output extended group element.
 * @return void
 */
void ge_p3_0(ge_p3 *h);

/**
 * @brief Double extended group element: r = 2 * p.
 *
 * @param r Output completed group element.
 * @param p Input extended group element.
 * @return void
 */
void ge_p3_dbl(ge_p1p1 *r, const ge_p3 *p);

/**
 * @brief Convert extended group element (ge_p3) to cached representation (ge_cached).
 *
 * @param r Output cached group element.
 * @param p Input extended group element.
 * @return void
 */
void ge_p3_to_cached(ge_cached *r, const ge_p3 *p);

/**
 * @brief Convert extended group element (ge_p3) to projective representation (ge_p2).
 *
 * @param r Output projective group element.
 * @param p Input extended group element.
 * @return void
 */
void ge_p3_to_p2(ge_p2 *r, const ge_p3 *p);

#endif // GE_H
