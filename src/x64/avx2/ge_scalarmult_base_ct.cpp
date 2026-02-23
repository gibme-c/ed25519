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
 * @file avx2/ge_scalarmult_base_ct.cpp
 * @brief AVX2 constant-time fixed-base scalar multiplication.
 *
 * Two compile-time paths:
 * - MSVC: radix-2^25.5 field ops (avoids uint128 struct) + AVX2 table selection
 * - GCC/Clang: standard fe51 chain ops + AVX2 table selection
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
#include "negative.h"
#include "x64/avx2/ge_avx2_select.h"

// clang-format off (must come after headers that define ge_precomp)
#include "../ge_precomp_base.inl"
// clang-format on

#if defined(_MSC_VER) || defined(ED25519_FORCE_AVX2_FMUL)
// ── MSVC path: radix-2^25.5 fe10-throughout ──

#include "x64/avx2/fe10_avx2_chain.h"

// ── fe10-throughout ge operations ──

static FE10_AVX2_FORCE_INLINE void ge_madd_10(ge_p1p1_10 *r, const ge_p3_10 *p, const ge_precomp_10 *q)
{
    fe10 t0;
    fe10_add(r->X, p->Y, p->X);
    fe10_sub(r->Y, p->Y, p->X);
    fe10_avx2_chain_mul(r->Z, r->X, q->yplusx);
    fe10_avx2_chain_mul(r->Y, r->Y, q->yminusx);
    fe10_avx2_chain_mul(r->T, q->xy2d, p->T);
    fe10_add(t0, p->Z, p->Z);
    fe10_sub(r->X, r->Z, r->Y);
    fe10_add(r->Y, r->Z, r->Y);
    fe10_add(r->Z, t0, r->T);
    fe10_sub(r->T, t0, r->T);
}

static FE10_AVX2_FORCE_INLINE void ge_p2_dbl_10(ge_p1p1_10 *r, const ge_p2_10 *p)
{
    fe10 t0;
    fe10_avx2_chain_sq(r->X, p->X);
    fe10_avx2_chain_sq(r->Z, p->Y);
    fe10_avx2_chain_sq2(r->T, p->Z);
    fe10_add(r->Y, p->X, p->Y);
    fe10_avx2_chain_sq(t0, r->Y);
    fe10_add(r->Y, r->Z, r->X);
    fe10_sub(r->Z, r->Z, r->X);
    fe10_sub(r->X, t0, r->Y);
    fe10_sub(r->T, r->T, r->Z);
}

static FE10_AVX2_FORCE_INLINE void ge_p3_dbl_10(ge_p1p1_10 *r, const ge_p3_10 *p)
{
    ge_p2_10 q;
    fe10_copy(q.X, p->X);
    fe10_copy(q.Y, p->Y);
    fe10_copy(q.Z, p->Z);
    ge_p2_dbl_10(r, &q);
}

static FE10_AVX2_FORCE_INLINE void ge_p1p1_to_p2_10(ge_p2_10 *r, const ge_p1p1_10 *p)
{
    fe10_avx2_chain_mul(r->X, p->X, p->T);
    fe10_avx2_chain_mul(r->Y, p->Y, p->Z);
    fe10_avx2_chain_mul(r->Z, p->Z, p->T);
}

static FE10_AVX2_FORCE_INLINE void ge_p1p1_to_p3_10(ge_p3_10 *r, const ge_p1p1_10 *p)
{
    fe10_avx2_chain_mul(r->X, p->X, p->T);
    fe10_avx2_chain_mul(r->Y, p->Y, p->Z);
    fe10_avx2_chain_mul(r->Z, p->Z, p->T);
    fe10_avx2_chain_mul(r->T, p->X, p->Y);
}

static FE10_AVX2_FORCE_INLINE void ge_p1p1_10_to_fe51(ge_p1p1 *out, const ge_p1p1_10 *in)
{
    fe10_to_fe51(out->X, in->X);
    fe10_to_fe51(out->Y, in->Y);
    fe10_to_fe51(out->Z, in->Z);
    fe10_to_fe51(out->T, in->T);
}

#else
// ── GCC/Clang path: standard fe51 chain ops + AVX2 table selection ──

#include "x64/fe51_chain.h"

static inline __attribute__((always_inline)) void ge_madd_avx2(ge_p1p1 *r, const ge_p3 *p, const ge_precomp *q)
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

static inline __attribute__((always_inline)) void ge_p2_dbl_avx2(ge_p1p1 *r, const ge_p2 *p)
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

static inline __attribute__((always_inline)) void ge_p3_dbl_avx2(ge_p1p1 *r, const ge_p3 *p)
{
    ge_p2 q;
    fe_copy(q.X, p->X);
    fe_copy(q.Y, p->Y);
    fe_copy(q.Z, p->Z);
    ge_p2_dbl_avx2(r, &q);
}

static inline __attribute__((always_inline)) void ge_p1p1_to_p2_avx2(ge_p2 *r, const ge_p1p1 *p)
{
    fe51_chain_mul(r->X, p->X, p->T);
    fe51_chain_mul(r->Y, p->Y, p->Z);
    fe51_chain_mul(r->Z, p->Z, p->T);
}

static inline __attribute__((always_inline)) void ge_p1p1_to_p3_avx2(ge_p3 *r, const ge_p1p1 *p)
{
    fe51_chain_mul(r->X, p->X, p->T);
    fe51_chain_mul(r->Y, p->Y, p->Z);
    fe51_chain_mul(r->Z, p->Z, p->T);
    fe51_chain_mul(r->T, p->X, p->Y);
}

#endif // _MSC_VER || ED25519_FORCE_AVX2_FMUL

// Table selection: AVX2 cmov on the fe51 ge_precomp table, then convert to fe10 if needed
static void select_avx2(ge_precomp *t, int pos, signed char b)
{
    unsigned char bnegative = negative(b);
    unsigned char babs = b - (((-bnegative) & b) << 1);

    ge_precomp_0_avx2(t);
    ge_precomp_cmov_avx2(t, &ge_base[pos][0], equal(babs, 1));
    ge_precomp_cmov_avx2(t, &ge_base[pos][1], equal(babs, 2));
    ge_precomp_cmov_avx2(t, &ge_base[pos][2], equal(babs, 3));
    ge_precomp_cmov_avx2(t, &ge_base[pos][3], equal(babs, 4));
    ge_precomp_cmov_avx2(t, &ge_base[pos][4], equal(babs, 5));
    ge_precomp_cmov_avx2(t, &ge_base[pos][5], equal(babs, 6));
    ge_precomp_cmov_avx2(t, &ge_base[pos][6], equal(babs, 7));
    ge_precomp_cmov_avx2(t, &ge_base[pos][7], equal(babs, 8));
    ge_precomp_cneg_avx2(t, bnegative);
}

/*
h = a * B
where a = a[0]+256*a[1]+...+256^31 a[31]
B is the Ed25519 base point (x,4/5) with x positive.

Preconditions:
  a[31] <= 127
*/

void ge_scalarmult_base_ct_avx2(ge_p1p1 *r, const unsigned char *a)
{
    signed char e[64];
    signed char carry;
    int i;

    for (i = 0; i < 32; ++i)
    {
        e[2 * i + 0] = (a[i] >> 0) & 15;
        e[2 * i + 1] = (a[i] >> 4) & 15;
    }

    carry = 0;
    for (i = 0; i < 63; ++i)
    {
        e[i] += carry;
        carry = e[i] + 8;
        carry >>= 4;
        e[i] -= carry << 4;
    }
    e[63] += carry;

#if defined(_MSC_VER) || defined(ED25519_FORCE_AVX2_FMUL)
    // ── MSVC fe10-throughout path ──
    // Select from fe51 table with AVX2 cmov, convert selected entry to fe10,
    // then do all arithmetic in fe10.

    ge_p3_10 h;
    ge_p1p1_10 r10;
    ge_precomp t;
    ge_precomp_10 t10;

    // h = identity in fe10
    for (int j = 0; j < 10; j++)
    {
        h.X[j] = 0;
        h.Y[j] = 0;
        h.Z[j] = 0;
        h.T[j] = 0;
    }
    h.Y[0] = 1;
    h.Z[0] = 1;

    for (i = 1; i < 64; i += 2)
    {
        select_avx2(&t, i / 2, e[i]);
        // Convert selected precomp from fe51 to fe10
        fe51_to_fe10(t10.yplusx, t.yplusx);
        fe51_to_fe10(t10.yminusx, t.yminusx);
        fe51_to_fe10(t10.xy2d, t.xy2d);
        ge_madd_10(&r10, &h, &t10);
        ge_p1p1_to_p3_10(&h, &r10);
    }

    ge_p2_10 s;
    ge_p3_dbl_10(&r10, &h);
    ge_p1p1_to_p2_10(&s, &r10);
    ge_p2_dbl_10(&r10, &s);
    ge_p1p1_to_p2_10(&s, &r10);
    ge_p2_dbl_10(&r10, &s);
    ge_p1p1_to_p2_10(&s, &r10);
    ge_p2_dbl_10(&r10, &s);
    ge_p1p1_to_p3_10(&h, &r10);

    for (i = 0; i < 64; i += 2)
    {
        select_avx2(&t, i / 2, e[i]);
        fe51_to_fe10(t10.yplusx, t.yplusx);
        fe51_to_fe10(t10.yminusx, t.yminusx);
        fe51_to_fe10(t10.xy2d, t.xy2d);
        ge_madd_10(&r10, &h, &t10);
        ge_p1p1_to_p3_10(&h, &r10);
    }

    // Convert final result to fe51
    ge_p1p1_10_to_fe51(r, &r10);

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(&carry, sizeof(carry));
    ed25519_secure_erase(&s, sizeof(s));
    ed25519_secure_erase(&t, sizeof(t));
    ed25519_secure_erase(&t10, sizeof(t10));
    ed25519_secure_erase(&h, sizeof(h));
    ed25519_secure_erase(&r10, sizeof(r10));
#else
    // ── GCC/Clang path: standard fe51 chain ops + AVX2 table selection ──
    ge_p2 s;
    ge_precomp t;
    ge_p3 h;

    ge_p3_0(&h);
    for (i = 1; i < 64; i += 2)
    {
        select_avx2(&t, i / 2, e[i]);
        ge_madd_avx2(r, &h, &t);
        ge_p1p1_to_p3_avx2(&h, r);
    }

    ge_p3_dbl_avx2(r, &h);
    ge_p1p1_to_p2_avx2(&s, r);
    ge_p2_dbl_avx2(r, &s);
    ge_p1p1_to_p2_avx2(&s, r);
    ge_p2_dbl_avx2(r, &s);
    ge_p1p1_to_p2_avx2(&s, r);
    ge_p2_dbl_avx2(r, &s);
    ge_p1p1_to_p3_avx2(&h, r);

    for (i = 0; i < 64; i += 2)
    {
        select_avx2(&t, i / 2, e[i]);
        ge_madd_avx2(r, &h, &t);
        ge_p1p1_to_p3_avx2(&h, r);
    }

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(&carry, sizeof(carry));
    ed25519_secure_erase(&s, sizeof(s));
    ed25519_secure_erase(&t, sizeof(t));
    ed25519_secure_erase(&h, sizeof(h));
#endif
}
