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

#include "ref10_scalarmult.h"

#include "equal.h"
#include "fe_copy.h"
#include "fe_neg.h"
#include "ge_add.h"
#include "ge_cached_0.h"
#include "ge_cached_cmov.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p2_0.h"
#include "ge_p2_dbl.h"
#include "ge_p3_to_cached.h"
#include "negative.h"

// adapted from ge_scalarmult_base.c

/*
h = a * A
where a = a[0]+256*a[1]+...+256^31 a[31]
A is a public point

Preconditions:
  a[31] <= 127
*/
void ref10_scalarmult(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A)
{
    signed char e[64];
    int carry, carry2, i;
    ge_cached Ai[8]; /* 1 * A, 2 * A, ..., 8 * A */
    ge_p3 u;

    carry = 0; /* 0..1 */
    for (i = 0; i < 31; i++)
    {
        carry += a[i]; /* 0..256 */
        carry2 = (carry + 8) >> 4; /* 0..16 */
        e[2 * i] = carry - (carry2 << 4); /* -8..7 */
        carry = (carry2 + 8) >> 4; /* 0..1 */
        e[2 * i + 1] = carry2 - (carry << 4); /* -8..7 */
    }
    carry += a[31]; /* 0..128 */
    carry2 = (carry + 8) >> 4; /* 0..8 */
    e[62] = carry - (carry2 << 4); /* -8..7 */
    e[63] = carry2; /* 0..8 */

    ge_p3_to_cached(&Ai[0], A);
    for (i = 0; i < 7; i++)
    {
        ge_add(t, A, &Ai[i]);
        ge_p1p1_to_p3(&u, t);
        ge_p3_to_cached(&Ai[i + 1], &u);
    }

    ge_p2 r;

    ge_p2_0(&r);
    for (i = 63; i >= 0; i--)
    {
        signed char b = e[i];
        unsigned char bnegative = negative(b);
        unsigned char babs = b - (((-bnegative) & b) << 1);
        ge_cached cur, minuscur;
        ge_p2_dbl(t, &r);
        ge_p1p1_to_p2(&r, t);
        ge_p2_dbl(t, &r);
        ge_p1p1_to_p2(&r, t);
        ge_p2_dbl(t, &r);
        ge_p1p1_to_p2(&r, t);
        ge_p2_dbl(t, &r);
        ge_p1p1_to_p3(&u, t);
        ge_cached_0(&cur);
        ge_cached_cmov(&cur, &Ai[0], equal(babs, 1));
        ge_cached_cmov(&cur, &Ai[1], equal(babs, 2));
        ge_cached_cmov(&cur, &Ai[2], equal(babs, 3));
        ge_cached_cmov(&cur, &Ai[3], equal(babs, 4));
        ge_cached_cmov(&cur, &Ai[4], equal(babs, 5));
        ge_cached_cmov(&cur, &Ai[5], equal(babs, 6));
        ge_cached_cmov(&cur, &Ai[6], equal(babs, 7));
        ge_cached_cmov(&cur, &Ai[7], equal(babs, 8));
        fe_copy(minuscur.YplusX, cur.YminusX);
        fe_copy(minuscur.YminusX, cur.YplusX);
        fe_copy(minuscur.Z, cur.Z);
        fe_neg(minuscur.T2d, cur.T2d);
        ge_cached_cmov(&cur, &minuscur, bnegative);
        ge_add(t, &u, &cur);
        ge_p1p1_to_p2(&r, t);
    }
}
