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
 * @file ge_p3_to_cached.h
 * @brief Convert an extended point to cached representation.
 *
 * Precomputes Y+X, Y-X, Z, and 2*d*T from a ge_p3 point. These values are
 * exactly what ge_add and ge_sub need from the second operand, so computing
 * them once and reusing them saves work when the same point is added
 * multiple times (as in scalar multiplication with a precomputed table).
 */

#ifndef ED25519_GE_P3_TO_CACHED_H
#define ED25519_GE_P3_TO_CACHED_H

#include "fe_add.h"
#include "fe_copy.h"
#include "fe_sub.h"
#include "ge.h"

/**
 * @brief Converts ge_p3 to ge_cached for fast repeated addition.
 *
 * Computes Y+X, Y-X, Z, and T*2d.
 *
 * @param r Output cached point.
 * @param p Input extended point.
 */
#if ED25519_PLATFORM_64BIT
#include "x64/fe51_chain.h"
static const fe ge_p3_to_cached_fe_d2 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};
static inline void ge_p3_to_cached(ge_cached *r, const ge_p3 *p)
{
    fe_add(r->YplusX, p->Y, p->X);
    fe_sub(r->YminusX, p->Y, p->X);
    fe_copy(r->Z, p->Z);
    fe51_chain_mul(r->T2d, p->T, ge_p3_to_cached_fe_d2);
}
#else
#include "portable/fe25_chain.h"
static const fe ge_p3_to_cached_fe_d2 =
    {-21827239, -5839606, -30745221, 13898782, 229458, 15978800, -12551817, -6495438, 29715968, 9444199};
static inline void ge_p3_to_cached(ge_cached *r, const ge_p3 *p)
{
    fe_add(r->YplusX, p->Y, p->X);
    fe_sub(r->YminusX, p->Y, p->X);
    fe_copy(r->Z, p->Z);
    fe25_chain_mul(r->T2d, p->T, ge_p3_to_cached_fe_d2);
}
#endif

#endif // ED25519_GE_P3_TO_CACHED_H
