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
 * @file ge_add.h
 * @brief Point addition on the Ed25519 curve.
 *
 * Adds an extended point (ge_p3) and a cached point (ge_cached), producing a
 * completed point (ge_p1p1). The cached representation has Y+X, Y-X, Z, and
 * 2dT precomputed, which saves several multiplications compared to adding two
 * raw extended points. This is the "unified addition" formula from Hisil et
 * al. (2008) -- it works correctly even when the two points are equal, unlike
 * classical formulas that break on doubling.
 */

#ifndef ED25519_GE_ADD_H
#define ED25519_GE_ADD_H

#include "fe_add.h"
#include "fe_sub.h"
#include "ge.h"

/**
 * @brief Adds an extended point and a cached point: r = p + q.
 *
 * @param r Output completed point.
 * @param p Input extended point.
 * @param q Input cached point.
 */
#if ED25519_PLATFORM_64BIT
void ge_add_x64(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q);

#if defined(_MSC_VER)
static inline void ge_add(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q)
{
    ge_add_x64(r, p, q);
}
#else
#include "x64/fe51_chain.h"
static inline void ge_add(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe51_chain_mul(r->Z, r->X, q->YplusX);
    fe51_chain_mul(r->Y, r->Y, q->YminusX);
    fe51_chain_mul(r->T, q->T2d, p->T);
    fe51_chain_mul(r->X, p->Z, q->Z);
    fe_add(t0, r->X, r->X);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_add(r->Z, t0, r->T);
    fe_sub(r->T, t0, r->T);
}
#endif
#else
#include "portable/fe25_chain.h"
static inline void ge_add(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe25_chain_mul(r->Z, r->X, q->YplusX);
    fe25_chain_mul(r->Y, r->Y, q->YminusX);
    fe25_chain_mul(r->T, q->T2d, p->T);
    fe25_chain_mul(r->X, p->Z, q->Z);
    fe_add(t0, r->X, r->X);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_add(r->Z, t0, r->T);
    fe_sub(r->T, t0, r->T);
}
#endif

#endif // ED25519_GE_ADD_H
