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
 * @file ge_p2_dbl.h
 * @brief Point doubling from projective coordinates.
 *
 * Doubles a projective point (ge_p2), producing a completed point (ge_p1p1).
 * Doubling starts from the simpler p2 form because the T coordinate isn't
 * needed -- the doubling formula only uses X, Y, and Z. This saves one field
 * multiplication compared to doubling from extended (p3) coordinates.
 */

#ifndef ED25519_GE_P2_DBL_H
#define ED25519_GE_P2_DBL_H

#include "fe_add.h"
#include "fe_sub.h"
#include "ge.h"

/**
 * @brief Doubles a projective point: r = 2 * p.
 *
 * @param r Output completed point.
 * @param p Input projective point.
 */
#if ED25519_PLATFORM_64BIT
void ge_p2_dbl_x64(ge_p1p1 *r, const ge_p2 *p);

#if defined(_MSC_VER)
static inline void ge_p2_dbl(ge_p1p1 *r, const ge_p2 *p)
{
    ge_p2_dbl_x64(r, p);
}
#else
#include "x64/fe51_chain.h"
static inline void ge_p2_dbl(ge_p1p1 *r, const ge_p2 *p)
{
    fe t0;
    fe51_chain_sq(r->X, p->X);
    fe51_chain_sq(r->Z, p->Y);
    fe51_chain_sq2(r->T, p->Z);
    fe_add(r->Y, p->X, p->Y);
    fe51_chain_sq(t0, r->Y);
    fe_add(r->Y, r->Z, r->X);
    fe_sub(r->Z, r->Z, r->X);
    fe_sub(r->X, t0, r->Y);
    fe_sub(r->T, r->T, r->Z);
}
#endif
#else
#include "portable/fe25_chain.h"
static inline void ge_p2_dbl(ge_p1p1 *r, const ge_p2 *p)
{
    fe t0;
    fe25_chain_sq(r->X, p->X);
    fe25_chain_sq(r->Z, p->Y);
    fe25_chain_sq2(r->T, p->Z);
    fe_add(r->Y, p->X, p->Y);
    fe25_chain_sq(t0, r->Y);
    fe_add(r->Y, r->Z, r->X);
    fe_sub(r->Z, r->Z, r->X);
    fe_sub(r->X, t0, r->Y);
    fe_sub(r->T, r->T, r->Z);
}
#endif

#endif // ED25519_GE_P2_DBL_H
