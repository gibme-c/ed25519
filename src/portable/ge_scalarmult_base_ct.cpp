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
#include "ed25519_platform.h"
#if !ED25519_PLATFORM_64BIT
#include "ed25519_secure_erase.h"
#include "equal.h"
#include "fe_copy.h"
#include "fe_neg.h"
#include "ge_madd.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p2_dbl.h"
#include "ge_p3_0.h"
#include "ge_p3_dbl.h"
#include "ge_precomp_0.h"
#include "ge_precomp_base.inl"
#include "ge_precomp_cmov.h"
#include "negative.h"
#include "portable/ge_scalarmult_base_ct.h"

static void select_portable(ge_precomp *t, int pos, signed char b)
{
    ge_precomp minust;
    unsigned char bnegative = negative(b);
    unsigned char babs = b - (((-bnegative) & b) << 1);

    ge_precomp_0(t);
    ge_precomp_cmov(t, &ge_base[pos][0], equal(babs, 1));
    ge_precomp_cmov(t, &ge_base[pos][1], equal(babs, 2));
    ge_precomp_cmov(t, &ge_base[pos][2], equal(babs, 3));
    ge_precomp_cmov(t, &ge_base[pos][3], equal(babs, 4));
    ge_precomp_cmov(t, &ge_base[pos][4], equal(babs, 5));
    ge_precomp_cmov(t, &ge_base[pos][5], equal(babs, 6));
    ge_precomp_cmov(t, &ge_base[pos][6], equal(babs, 7));
    ge_precomp_cmov(t, &ge_base[pos][7], equal(babs, 8));
    fe_copy(minust.yplusx, t->yminusx);
    fe_copy(minust.yminusx, t->yplusx);
    fe_neg(minust.xy2d, t->xy2d);
    ge_precomp_cmov(t, &minust, bnegative);
}

/*
h = a * B
where a = a[0]+256*a[1]+...+256^31 a[31]
B is the Ed25519 base point (x,4/5) with x positive.

Preconditions:
  a[31] <= 127
*/

void ge_scalarmult_base_ct_portable(ge_p1p1 *r, const unsigned char *a)
{
    signed char e[64];
    signed char carry;
    ge_p2 s;
    ge_precomp t;
    int i;

    for (i = 0; i < 32; ++i)
    {
        e[2 * i + 0] = (a[i] >> 0) & 15;
        e[2 * i + 1] = (a[i] >> 4) & 15;
    }
    /* each e[i] is between 0 and 15 */
    /* e[63] is between 0 and 7 */

    carry = 0;
    for (i = 0; i < 63; ++i)
    {
        e[i] += carry;
        carry = e[i] + 8;
        carry >>= 4;
        e[i] -= carry << 4;
    }
    e[63] += carry;
    /* each e[i] is between -8 and 8 */

    ge_p3 h;

    ge_p3_0(&h);
    for (i = 1; i < 64; i += 2)
    {
        select_portable(&t, i / 2, e[i]);
        ge_madd(r, &h, &t);
        ge_p1p1_to_p3(&h, r);
    }

    ge_p3_dbl(r, &h);
    ge_p1p1_to_p2(&s, r);
    ge_p2_dbl(r, &s);
    ge_p1p1_to_p2(&s, r);
    ge_p2_dbl(r, &s);
    ge_p1p1_to_p2(&s, r);
    ge_p2_dbl(r, &s);
    ge_p1p1_to_p3(&h, r);

    for (i = 0; i < 64; i += 2)
    {
        select_portable(&t, i / 2, e[i]);
        ge_madd(r, &h, &t);
        ge_p1p1_to_p3(&h, r);
    }

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(&s, sizeof(s));
    ed25519_secure_erase(&t, sizeof(t));
    ed25519_secure_erase(&h, sizeof(h));
    ed25519_secure_erase(&carry, sizeof(carry));
}
#endif
