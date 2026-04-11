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
 * @file avx2/ge_scalarmult_ct.cpp
 * @brief AVX2 constant-time variable-base scalar multiplication.
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
#include "negative.h"
#include "x64/avx2/ge_avx2_select.h"

#if defined(_MSC_VER) || defined(ED25519_FORCE_AVX2_FMUL)
// ── MSVC path: radix-2^25.5 fe10-throughout ──
// All intermediates stay in fe10 format. Convert fe51→fe10 once at entry,
// fe10→fe51 once at exit. No per-ge-operation conversions.

#include "x64/avx2/fe10_avx2_chain.h"

// d2 = 2*d in fe51 format, converted to fe10 at point of use.
static const fe ge_p3_to_cached_avx2_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

// ── fe10-throughout ge operations ──

static FE10_AVX2_FORCE_INLINE void ge_add_10(ge_p1p1_10 *r, const ge_p3_10 *p, const ge_cached_10 *q)
{
    fe10 t0;
    fe10_add(r->X, p->Y, p->X);
    fe10_sub(r->Y, p->Y, p->X);
    fe10_avx2_chain_mul(r->Z, r->X, q->YplusX);
    fe10_avx2_chain_mul(r->Y, r->Y, q->YminusX);
    fe10_avx2_chain_mul(r->T, q->T2d, p->T);
    fe10_avx2_chain_mul(r->X, p->Z, q->Z);
    fe10_add(t0, r->X, r->X);
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

static FE10_AVX2_FORCE_INLINE void ge_p3_to_cached_10(ge_cached_10 *r, const ge_p3_10 *p, const fe10 d2_10)
{
    fe10_add(r->YplusX, p->Y, p->X);
    fe10_sub(r->YminusX, p->Y, p->X);
    fe10_copy(r->Z, p->Z);
    fe10_avx2_chain_mul(r->T2d, p->T, d2_10);
}

// Adapter functions: fe51 ↔ fe10 at scalarmult boundaries only

static FE10_AVX2_FORCE_INLINE void ge_p3_to_p3_10(ge_p3_10 *out, const ge_p3 *in)
{
    fe51_to_fe10(out->X, in->X);
    fe51_to_fe10(out->Y, in->Y);
    fe51_to_fe10(out->Z, in->Z);
    fe51_to_fe10(out->T, in->T);
}

static FE10_AVX2_FORCE_INLINE void ge_p1p1_10_to_fe51(ge_p1p1 *out, const ge_p1p1_10 *in)
{
    fe10_to_fe51(out->X, in->X);
    fe10_to_fe51(out->Y, in->Y);
    fe10_to_fe51(out->Z, in->Z);
    fe10_to_fe51(out->T, in->T);
}

#else
// ── GCC/Clang path: standard fe51 chain ops + AVX2 cmov only ──
// __int128 fe51 mul/sq is already fast. Only replace table selection with AVX2.

#include "x64/fe51_chain.h"

static inline __attribute__((always_inline)) void
    ge_add_avx2(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q, fe * /*scratch*/)
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

static const fe ge_p3_to_cached_avx2_d2 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

static inline __attribute__((always_inline)) void ge_p3_to_cached_avx2(ge_cached *r, const ge_p3 *p)
{
    fe_add(r->YplusX, p->Y, p->X);
    fe_sub(r->YminusX, p->Y, p->X);
    fe_copy(r->Z, p->Z);
    fe51_chain_mul(r->T2d, p->T, ge_p3_to_cached_avx2_d2);
}

#endif // _MSC_VER || ED25519_FORCE_AVX2_FMUL

/*
h = a * A
where a = a[0]+256*a[1]+...+256^31 a[31]
A is a public point

Preconditions:
  a[31] <= 127
*/
void ge_scalarmult_avx2_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A)
{
    signed char e[64];
    int carry, carry2, i;

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

#if defined(_MSC_VER) || defined(ED25519_FORCE_AVX2_FMUL)
    // ── MSVC fe10-throughout path ──
    // All intermediates stay in fe10 format. Convert once at entry/exit.

    // Convert d2 constant to fe10
    fe10 d2_10;
    fe51_to_fe10(d2_10, ge_p3_to_cached_avx2_d2_fe51);

    // Convert input A to fe10
    ge_p3_10 A10;
    ge_p3_to_p3_10(&A10, A);

    // Build table in fe10 format
    alignas(64) ge_cached_10 Ai[8];
    ge_p3_10 u;
    ge_p1p1_10 t10;

    ge_p3_to_cached_10(&Ai[0], &A10, d2_10);
    for (i = 0; i < 7; i++)
    {
        ge_add_10(&t10, &A10, &Ai[i]);
        ge_p1p1_to_p3_10(&u, &t10);
        ge_p3_to_cached_10(&Ai[i + 1], &u, d2_10);
    }

    ge_p2_10 r;
    ge_cached_10 cur;
    // Initialize r to identity in fe10: X=0, Y=1, Z=1
    for (int j = 0; j < 10; j++)
        r.X[j] = 0;
    for (int j = 0; j < 10; j++)
        r.Y[j] = 0;
    for (int j = 0; j < 10; j++)
        r.Z[j] = 0;
    r.Y[0] = 1;
    r.Z[0] = 1;

    for (i = 63; i >= 0; i--)
    {
        signed char b = e[i];
        unsigned char bnegative = negative(b);
        // Branchless |b| via XOR-subtract trick, computed in unsigned to avoid
        // UB from left-shifting a negative int.
        unsigned char babs = (unsigned char)(((unsigned int)(int)b ^ (0u - (unsigned int)bnegative)) + bnegative);
        ge_p2_dbl_10(&t10, &r);
        ge_p1p1_to_p2_10(&r, &t10);
        ge_p2_dbl_10(&t10, &r);
        ge_p1p1_to_p2_10(&r, &t10);
        ge_p2_dbl_10(&t10, &r);
        ge_p1p1_to_p2_10(&r, &t10);
        ge_p2_dbl_10(&t10, &r);
        ge_p1p1_to_p3_10(&u, &t10);
        ge_cached_0_10_avx2(&cur);
        ge_cached_cmov_10_avx2(&cur, &Ai[0], equal(babs, 1));
        ge_cached_cmov_10_avx2(&cur, &Ai[1], equal(babs, 2));
        ge_cached_cmov_10_avx2(&cur, &Ai[2], equal(babs, 3));
        ge_cached_cmov_10_avx2(&cur, &Ai[3], equal(babs, 4));
        ge_cached_cmov_10_avx2(&cur, &Ai[4], equal(babs, 5));
        ge_cached_cmov_10_avx2(&cur, &Ai[5], equal(babs, 6));
        ge_cached_cmov_10_avx2(&cur, &Ai[6], equal(babs, 7));
        ge_cached_cmov_10_avx2(&cur, &Ai[7], equal(babs, 8));
        ge_cached_cneg_10_avx2(&cur, bnegative);
        ge_add_10(&t10, &u, &cur);
        ge_p1p1_to_p2_10(&r, &t10);
    }

    // Convert final p1p1 result back to fe51 for the caller
    // The last operation was ge_p1p1_to_p2_10 into r, but the caller
    // expects t (ge_p1p1). Re-do the last add to produce a ge_p1p1.
    // Actually, we need to output the ge_p1p1 from the last ge_add_10.
    // The loop always ends with ge_p1p1_to_p2_10(&r, &t10), so t10
    // holds the final ge_p1p1_10. Convert it.
    ge_p1p1_10_to_fe51(t, &t10);

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(&carry, sizeof(carry));
    ed25519_secure_erase(&carry2, sizeof(carry2));
    ed25519_secure_erase(Ai, sizeof(Ai));
    ed25519_secure_erase(&u, sizeof(u));
    ed25519_secure_erase(&r, sizeof(r));
    ed25519_secure_erase(&cur, sizeof(cur));
    ed25519_secure_erase(&t10, sizeof(t10));
#else
    // ── GCC/Clang path: standard fe51 chain ops + AVX2 cmov ──
    alignas(64) ge_cached Ai[8];
    ge_p3 u;

    ge_p3_to_cached_avx2(&Ai[0], A);
    for (i = 0; i < 7; i++)
    {
        ge_add_avx2(t, A, &Ai[i], (fe *)0);
        ge_p1p1_to_p3_avx2(&u, t);
        ge_p3_to_cached_avx2(&Ai[i + 1], &u);
    }

    ge_p2 r;
    ge_cached cur;

    ge_p2_0(&r);
    for (i = 63; i >= 0; i--)
    {
        signed char b = e[i];
        unsigned char bnegative = negative(b);
        // Branchless |b| via XOR-subtract trick, computed in unsigned to avoid
        // UB from left-shifting a negative int.
        unsigned char babs = (unsigned char)(((unsigned int)(int)b ^ (0u - (unsigned int)bnegative)) + bnegative);
        ge_p2_dbl_avx2(t, &r);
        ge_p1p1_to_p2_avx2(&r, t);
        ge_p2_dbl_avx2(t, &r);
        ge_p1p1_to_p2_avx2(&r, t);
        ge_p2_dbl_avx2(t, &r);
        ge_p1p1_to_p2_avx2(&r, t);
        ge_p2_dbl_avx2(t, &r);
        ge_p1p1_to_p3_avx2(&u, t);
        ge_cached_0_avx2(&cur);
        ge_cached_cmov_avx2(&cur, &Ai[0], equal(babs, 1));
        ge_cached_cmov_avx2(&cur, &Ai[1], equal(babs, 2));
        ge_cached_cmov_avx2(&cur, &Ai[2], equal(babs, 3));
        ge_cached_cmov_avx2(&cur, &Ai[3], equal(babs, 4));
        ge_cached_cmov_avx2(&cur, &Ai[4], equal(babs, 5));
        ge_cached_cmov_avx2(&cur, &Ai[5], equal(babs, 6));
        ge_cached_cmov_avx2(&cur, &Ai[6], equal(babs, 7));
        ge_cached_cmov_avx2(&cur, &Ai[7], equal(babs, 8));
        ge_cached_cneg_avx2(&cur, bnegative);
        ge_add_avx2(t, &u, &cur, (fe *)0);
        ge_p1p1_to_p2_avx2(&r, t);
    }

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(&carry, sizeof(carry));
    ed25519_secure_erase(&carry2, sizeof(carry2));
    ed25519_secure_erase(Ai, sizeof(Ai));
    ed25519_secure_erase(&u, sizeof(u));
    ed25519_secure_erase(&r, sizeof(r));
    ed25519_secure_erase(&cur, sizeof(cur));
#endif
}
