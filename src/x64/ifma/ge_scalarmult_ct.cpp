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
 * @file ifma/ge_scalarmult_ct.cpp
 * @brief AVX-512 IFMA constant-time variable-base scalar multiplication.
 *
 * Same algorithm as the x64 scalar path but uses IFMA field arithmetic
 * for the inner ge operations (add, double, p1p1_to_p2/p3, p3_to_cached).
 */

#include "ed25519_secure_erase.h"
#include "equal.h"
#include "fe_add.h"
#include "fe_copy.h"
#include "fe_neg.h"
#include "fe_sub.h"
#include "ge.h"
#include "ge_cached_0.h"
#include "ge_cached_cmov.h"
#include "ge_p2_0.h"
#include "negative.h"
#include "x64/ifma/fe_ifma_chain.h"

// IFMA ge operations - inlined here since this TU is compiled with AVX-512 flags

static FE_IFMA_FORCE_INLINE void ge_add_ifma(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe_ifma_chain_mul_nn(r->Z, r->X, q->YplusX);
    fe_ifma_chain_mul_nn(r->Y, r->Y, q->YminusX);
    fe_ifma_chain_mul_nn(r->T, q->T2d, p->T);
    fe_ifma_chain_mul_nn(r->X, p->Z, q->Z);
    fe_add(t0, r->X, r->X);
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

static const fe ge_p3_to_cached_ifma_d2 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

static FE_IFMA_FORCE_INLINE void ge_p3_to_cached_ifma(ge_cached *r, const ge_p3 *p)
{
    fe_add(r->YplusX, p->Y, p->X);
    fe_sub(r->YminusX, p->Y, p->X);
    fe_copy(r->Z, p->Z);
    fe_ifma_chain_mul_nn(r->T2d, p->T, ge_p3_to_cached_ifma_d2);
}

/*
h = a * A
where a = a[0]+256*a[1]+...+256^31 a[31]
A is a public point

Preconditions:
  a[31] <= 127
*/
void ge_scalarmult_ifma_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A)
{
    signed char e[64];
    int carry, carry2, i;
    alignas(64) ge_cached Ai[8]; /* 1 * A, 2 * A, ..., 8 * A */
    ge_p3 u;

    carry = 0; /* 0..1 */
    for (i = 0; i < 31; i++)
    {
        carry += a[i]; /* 0..256 */
        carry2 = (carry + 8) >> 4; /* 0..16 */
        e[2 * i] = (signed char)(carry - (carry2 << 4)); /* -8..7 */
        carry = (carry2 + 8) >> 4; /* 0..1 */
        e[2 * i + 1] = (signed char)(carry2 - (carry << 4)); /* -8..7 */
    }
    carry += a[31]; /* 0..128 */
    carry2 = (carry + 8) >> 4; /* 0..8 */
    e[62] = (signed char)(carry - (carry2 << 4)); /* -8..7 */
    e[63] = (signed char)carry2; /* 0..8 */

    ge_p3_to_cached_ifma(&Ai[0], A);
    for (i = 0; i < 7; i++)
    {
        ge_add_ifma(t, A, &Ai[i]);
        ge_p1p1_to_p3_ifma(&u, t);
        ge_p3_to_cached_ifma(&Ai[i + 1], &u);
    }

    ge_p2 r;
    ge_cached cur, minuscur;

    ge_p2_0(&r);
    for (i = 63; i >= 0; i--)
    {
        signed char b = e[i];
        unsigned char bnegative = negative(b);
        // Branchless |b| via XOR-subtract trick, computed in unsigned to avoid
        // UB from left-shifting a negative int.
        unsigned char babs = (unsigned char)(((unsigned int)(int)b ^ (0u - (unsigned int)bnegative)) + bnegative);
        ge_p2_dbl_ifma(t, &r);
        ge_p1p1_to_p2_ifma(&r, t);
        ge_p2_dbl_ifma(t, &r);
        ge_p1p1_to_p2_ifma(&r, t);
        ge_p2_dbl_ifma(t, &r);
        ge_p1p1_to_p2_ifma(&r, t);
        ge_p2_dbl_ifma(t, &r);
        ge_p1p1_to_p3_ifma(&u, t);
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
        ge_add_ifma(t, &u, &cur);
        ge_p1p1_to_p2_ifma(&r, t);
    }

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(&carry, sizeof(carry));
    ed25519_secure_erase(&carry2, sizeof(carry2));
    ed25519_secure_erase(Ai, sizeof(Ai));
    ed25519_secure_erase(&u, sizeof(u));
    ed25519_secure_erase(&r, sizeof(r));
    ed25519_secure_erase(&cur, sizeof(cur));
    ed25519_secure_erase(&minuscur, sizeof(minuscur));
}
