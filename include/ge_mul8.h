/**
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

/**
 * @file ge_mul8.h
 * @brief Multiply a point by the cofactor 8.
 *
 * Computes 8*A by doing 3 successive point doublings. The Ed25519 curve has
 * cofactor 8, meaning the full curve group has order 8*l. Multiplying by 8
 * "clears" the cofactor, projecting any curve point into the prime-order
 * subgroup. This is used in some protocol constructions (like cofactored
 * Diffie-Hellman) to avoid small-subgroup attacks.
 */

#ifndef ED25519_GE_MUL8_H
#define ED25519_GE_MUL8_H

#include "ge.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p2_dbl.h"

/**
 * @brief Multiplies a projective point by the cofactor 8: r = 8 * s.
 *
 * Performs three consecutive doublings.
 *
 * @param r Output completed point.
 * @param s Input projective point.
 */
static inline void ge_mul8(ge_p1p1 *r, const ge_p2 *t)
{
    ge_p2 u;
    ge_p2_dbl(r, t);
    ge_p1p1_to_p2(&u, r);
    ge_p2_dbl(r, &u);
    ge_p1p1_to_p2(&u, r);
    ge_p2_dbl(r, &u);
}

#endif // ED25519_GE_MUL8_H
