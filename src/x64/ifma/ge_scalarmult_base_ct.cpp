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
 * @file ifma/ge_scalarmult_base_ct.cpp
 * @brief AVX-512 IFMA constant-time fixed-base scalar multiplication.
 *
 * Same algorithm as the x64 scalar path but uses IFMA field arithmetic.
 */

#include "ed25519_secure_erase.h"
#include "equal.h"
#include "fe_add.h"
#include "fe_copy.h"
#include "fe_neg.h"
#include "fe_sub.h"
#include "ge.h"
#include "ge_p2_0.h"
#include "ge_p3_0.h"
#include "ge_precomp_0.h"
#include "ge_precomp_cmov.h"
#include "negative.h"
#include "x64/ifma/fe_ifma_chain.h"

// clang-format off (must come after headers that define ge_precomp)
#include "../ge_precomp_base.inl"
// clang-format on

// IFMA ge operations inlined here

static FE_IFMA_FORCE_INLINE void ge_madd_ifma(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe_ifma_chain_mul_nn(r->Z, r->X, q->yplusx);
    fe_ifma_chain_mul_nn(r->Y, r->Y, q->yminusx);
    fe_ifma_chain_mul_nn(r->T, q->xy2d, p->T);
    fe_add(t0, p->Z, p->Z);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_add(r->Z, t0, r->T);
    fe_normalize_weak(r->Z); // t0 ≤52 + r->T ≤51 → ≤53, must normalize for IFMA
    fe_sub(r->T, t0, r->T);
}

static FE_IFMA_FORCE_INLINE void ge_p2_dbl_ifma(ge_p1p1 *r, const ge_p2 *p)
{
    fe t0;
    fe_ifma_chain_sq_n(r->X, p->X);
    fe_ifma_chain_sq_n(r->Z, p->Y);
    fe_ifma_chain_sq2_n(r->T, p->Z);
    fe_add(r->Y, p->X, p->Y);
    fe_ifma_chain_sq_n(t0, r->Y);
    fe_add(r->Y, r->Z, r->X);
    fe_sub(r->Z, r->Z, r->X);
    fe_sub(r->X, t0, r->Y);
    fe_sub(r->T, r->T, r->Z);
}

static FE_IFMA_FORCE_INLINE void ge_p3_dbl_ifma(ge_p1p1 *r, const ge_p3 *p)
{
    ge_p2 q;
    fe_copy(q.X, p->X);
    fe_copy(q.Y, p->Y);
    fe_copy(q.Z, p->Z);
    ge_p2_dbl_ifma(r, &q);
}

static FE_IFMA_FORCE_INLINE void ge_p1p1_to_p2_ifma(ge_p2 *r, const ge_p1p1 *p)
{
    fe_ifma_chain_mul_nn(r->X, p->X, p->T);
    fe_ifma_chain_mul_nn(r->Y, p->Y, p->Z);
    fe_ifma_chain_mul_nn(r->Z, p->Z, p->T);
}

static FE_IFMA_FORCE_INLINE void ge_p1p1_to_p3_ifma(ge_p3 *r, const ge_p1p1 *p)
{
    fe_ifma_chain_mul_nn(r->X, p->X, p->T);
    fe_ifma_chain_mul_nn(r->Y, p->Y, p->Z);
    fe_ifma_chain_mul_nn(r->Z, p->Z, p->T);
    fe_ifma_chain_mul_nn(r->T, p->X, p->Y);
}

static void select_ifma(ge_precomp *t, int pos, signed char b)
{
    ge_precomp minust;
    unsigned char bnegative = negative(b);
    // Branchless |b| via XOR-subtract trick, computed in unsigned to avoid
    // UB from left-shifting a negative int.
    unsigned char babs = (unsigned char)(((unsigned int)(int)b ^ (0u - (unsigned int)bnegative)) + bnegative);

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

void ge_scalarmult_base_ct_ifma(ge_p1p1 *r, const unsigned char *a)
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
        select_ifma(&t, i / 2, e[i]);
        ge_madd_ifma(r, &h, &t);
        ge_p1p1_to_p3_ifma(&h, r);
    }

    ge_p3_dbl_ifma(r, &h);
    ge_p1p1_to_p2_ifma(&s, r);
    ge_p2_dbl_ifma(r, &s);
    ge_p1p1_to_p2_ifma(&s, r);
    ge_p2_dbl_ifma(r, &s);
    ge_p1p1_to_p2_ifma(&s, r);
    ge_p2_dbl_ifma(r, &s);
    ge_p1p1_to_p3_ifma(&h, r);

    for (i = 0; i < 64; i += 2)
    {
        select_ifma(&t, i / 2, e[i]);
        ge_madd_ifma(r, &h, &t);
        ge_p1p1_to_p3_ifma(&h, r);
    }

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(&carry, sizeof(carry));
    ed25519_secure_erase(&s, sizeof(s));
    ed25519_secure_erase(&t, sizeof(t));
    ed25519_secure_erase(&h, sizeof(h));
}
