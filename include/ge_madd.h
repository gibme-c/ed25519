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
 * @file ge_madd.h
 * @brief Mixed point addition on the Ed25519 curve.
 *
 * "Mixed" addition means one operand is a precomp point with Z=1 (affine).
 * This saves a couple of multiplications compared to the full ge_add. It's
 * used in fixed-base scalar multiplication where the base point table
 * entries are precomputed in affine form.
 */

#ifndef ED25519_GE_MADD_H
#define ED25519_GE_MADD_H

#include "fe_add.h"
#include "fe_sub.h"
#include "ge.h"

/**
 * @brief Mixed addition of extended and precomputed points: r = p + q.
 *
 * Uses the precomputed (y+x, y-x, 2dxy) form for cheaper addition.
 *
 * @param r Output completed point.
 * @param p Input extended point.
 * @param q Input precomputed point.
 */
#if ED25519_PLATFORM_64BIT
void ge_madd_x64(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q);

#if defined(_MSC_VER)
static inline void ge_madd(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q)
{
    ge_madd_x64(r, p, q);
}
#else
#include "x64/fe51_chain.h"
static inline void ge_madd(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe51_chain_mul(r->Z, r->X, q->yplusx);
    fe51_chain_mul(r->Y, r->Y, q->yminusx);
    fe51_chain_mul(r->T, q->xy2d, p->T);
    fe_add(t0, p->Z, p->Z);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_add(r->Z, t0, r->T);
    fe_sub(r->T, t0, r->T);
}
#endif
#else
#include "portable/fe25_chain.h"
static inline void ge_madd(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe25_chain_mul(r->Z, r->X, q->yplusx);
    fe25_chain_mul(r->Y, r->Y, q->yminusx);
    fe25_chain_mul(r->T, q->xy2d, p->T);
    fe_add(t0, p->Z, p->Z);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_add(r->Z, t0, r->T);
    fe_sub(r->T, t0, r->T);
}
#endif

#endif // ED25519_GE_MADD_H
