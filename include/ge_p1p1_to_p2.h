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
 * @file ge_p1p1_to_p2.h
 * @brief Convert a completed point to projective coordinates.
 *
 * "Completing" a point: ge_p1p1 stores the result of addition or doubling
 * in a deferred form where X and Y each have their own denominator (Z and T).
 * This conversion multiplies out to get a standard projective point: X*T,
 * Y*Z, Z*T. The T coordinate is discarded, giving a ge_p2. Costs 3 field
 * multiplications.
 */

#ifndef ED25519_GE_P1P1_TO_P2_H
#define ED25519_GE_P1P1_TO_P2_H

#include "fe_mul.h"
#include "ge.h"

/**
 * @brief Converts a ge_p1p1 (completed) point to ge_p2 (projective).
 *
 * Computes r.X = p.X * p.T, r.Y = p.Y * p.Z, r.Z = p.Z * p.T.
 *
 * @param r Output projective point.
 * @param p Input completed point.
 */
static inline void ge_p1p1_to_p2(ge_p2 *r, const ge_p1p1 *p)
{
    fe_mul(r->X, p->X, p->T);
    fe_mul(r->Y, p->Y, p->Z);
    fe_mul(r->Z, p->Z, p->T);
}

#endif // ED25519_GE_P1P1_TO_P2_H
