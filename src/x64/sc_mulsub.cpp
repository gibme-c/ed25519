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
 * @file x64/sc_mulsub.cpp
 * @brief x64 implementation of scalar multiply-subtract modulo l using Barrett reduction.
 *
 * Computes s = (c - a * b) mod l.
 * Algorithm: Barrett-reduce a*b to get t, then compute c - t with conditional add of L.
 * GCC/Clang: unsigned __int128 for 128-bit products.
 * MSVC: _umul128 + _addcarry_u64 intrinsic chain.
 */

#include "ed25519_platform.h"
#include "ed25519_secure_erase.h"

#include <cstdint>
#include <cstring>

/* Group order l as 4 x uint64_t (little-endian) */
static const uint64_t L[4] = {
    UINT64_C(0x5812631a5cf5d3ed),
    UINT64_C(0x14def9dea2f79cd6),
    UINT64_C(0x0000000000000000),
    UINT64_C(0x1000000000000000),
};

/* Barrett constant mu = floor(2^506 / l) as 4 x uint64_t (little-endian) */
static const uint64_t MU[4] = {
    UINT64_C(0x9fb673968c28b04c),
    UINT64_C(0xac84188574218ca6),
    UINT64_C(0xffffffffffffffff),
    UINT64_C(0x3fffffffffffffff),
};

#if ED25519_HAVE_INT128

/*
 * Multiply-accumulate: (c2:c1:c0) += a * b
 * Uses unsigned __int128 for 128-bit product.
 * The 3-word accumulator prevents overflow even with 4 products per column.
 */
static inline void mulacc(uint64_t &c0, uint64_t &c1, uint64_t &c2, uint64_t a, uint64_t b)
{
    unsigned __int128 prod = (unsigned __int128)a * b;
    unsigned __int128 acc = (unsigned __int128)c1 << 64 | c0;
    acc += prod;
    c0 = (uint64_t)acc;
    c1 = (uint64_t)(acc >> 64);
    c2 += (acc < prod) ? 1 : 0;
}

/*
 * 4x4 schoolbook multiply: out[8] = a[4] * b[4]
 * Uses 3-word accumulator (c2:c1:c0) per column to prevent overflow.
 */
static void mul4x4(uint64_t out[8], const uint64_t a[4], const uint64_t b[4])
{
    uint64_t c0, c1, c2;

    /* Column 0 */
    c0 = 0;
    c1 = 0;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[0]);
    out[0] = c0;

    /* Column 1 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[1]);
    mulacc(c0, c1, c2, a[1], b[0]);
    out[1] = c0;

    /* Column 2 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[2]);
    mulacc(c0, c1, c2, a[1], b[1]);
    mulacc(c0, c1, c2, a[2], b[0]);
    out[2] = c0;

    /* Column 3 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[3]);
    mulacc(c0, c1, c2, a[1], b[2]);
    mulacc(c0, c1, c2, a[2], b[1]);
    mulacc(c0, c1, c2, a[3], b[0]);
    out[3] = c0;

    /* Column 4 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[1], b[3]);
    mulacc(c0, c1, c2, a[2], b[2]);
    mulacc(c0, c1, c2, a[3], b[1]);
    out[4] = c0;

    /* Column 5 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[2], b[3]);
    mulacc(c0, c1, c2, a[3], b[2]);
    out[5] = c0;

    /* Column 6 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[3], b[3]);
    out[6] = c0;
    out[7] = c1;
}

/*
 * Compute low 4 limbs of q3 * L.
 * Exploits L[2] = 0 and L[3] = 0x1000000000000000 = 2^60.
 * Only the low 4 limbs are needed for the subtraction step.
 */
static void mul_q3_L_low(uint64_t out[4], const uint64_t q3[4])
{
    uint64_t c0, c1, c2;

    /* Column 0: q3[0]*L[0] */
    c0 = 0;
    c1 = 0;
    c2 = 0;
    mulacc(c0, c1, c2, q3[0], L[0]);
    out[0] = c0;

    /* Column 1: q3[0]*L[1] + q3[1]*L[0] */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, q3[0], L[1]);
    mulacc(c0, c1, c2, q3[1], L[0]);
    out[1] = c0;

    /* Column 2: q3[1]*L[1] + q3[2]*L[0] (L[2]=0 terms vanish) */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, q3[1], L[1]);
    mulacc(c0, c1, c2, q3[2], L[0]);
    out[2] = c0;

    /* Column 3: q3[2]*L[1] + q3[3]*L[0] + q3[0]*L[3] (L[2]=0 terms vanish)
     * L[3] = 2^60, so q3[0]*L[3] = q3[0] << 60 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, q3[2], L[1]);
    mulacc(c0, c1, c2, q3[3], L[0]);
    /* Add q3[0] << 60 = (q3[0] >> 4) : (q3[0] << 60) split across c1:c0 */
    {
        unsigned __int128 acc = (unsigned __int128)c1 << 64 | c0;
        acc += ((unsigned __int128)q3[0] << 60);
        c0 = (uint64_t)acc;
    }
    out[3] = c0;
}

/*
 * 4-limb subtraction: out = a - b (mod 2^256).
 * Returns borrow (0 or 1).
 */
static uint64_t sub4(uint64_t out[4], const uint64_t a[4], const uint64_t b[4])
{
    __int128 acc = (__int128)a[0] - b[0];
    out[0] = (uint64_t)acc;
    acc = (__int128)a[1] - b[1] + (int64_t)(acc >> 64);
    out[1] = (uint64_t)acc;
    acc = (__int128)a[2] - b[2] + (int64_t)(acc >> 64);
    out[2] = (uint64_t)acc;
    acc = (__int128)a[3] - b[3] + (int64_t)(acc >> 64);
    out[3] = (uint64_t)acc;
    /* Return sign bit: 0 if no borrow, 1 if borrow */
    return (uint64_t)((uint64_t)(acc >> 64) >> 63);
}

/*
 * 4-limb addition: out = a + b (mod 2^256).
 * Returns carry (0 or 1).
 */
static uint64_t add4(uint64_t out[4], const uint64_t a[4], const uint64_t b[4])
{
    unsigned __int128 acc = (unsigned __int128)a[0] + b[0];
    out[0] = (uint64_t)acc;
    acc = (unsigned __int128)a[1] + b[1] + (uint64_t)(acc >> 64);
    out[1] = (uint64_t)acc;
    acc = (unsigned __int128)a[2] + b[2] + (uint64_t)(acc >> 64);
    out[2] = (uint64_t)acc;
    acc = (unsigned __int128)a[3] + b[3] + (uint64_t)(acc >> 64);
    out[3] = (uint64_t)acc;
    return (uint64_t)(acc >> 64);
}

void sc_mulsub_x64(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c)
{
    uint64_t av[4], bv[4], cv[4];
    uint64_t product[8];
    uint64_t q1[5], q2[9], q3[4];
    uint64_t r1[4], r2[4], r[4], t[4], d[4];

    std::memcpy(av, a, 32);
    std::memcpy(bv, b, 32);
    std::memcpy(cv, c, 32);

    /* Mask inputs to 253 bits (Barrett requires product < 2^506) */
    av[3] &= UINT64_C(0x1FFFFFFFFFFFFFFF);
    bv[3] &= UINT64_C(0x1FFFFFFFFFFFFFFF);
    cv[3] &= UINT64_C(0x1FFFFFFFFFFFFFFF);

    /* Step 1: Schoolbook multiply product = av * bv (8 limbs) */
    mul4x4(product, av, bv);

    /* Step 2: q1 = product >> 252 (shift right by 252 = 3*64 + 60) */
    q1[0] = (product[3] >> 60) | (product[4] << 4);
    q1[1] = (product[4] >> 60) | (product[5] << 4);
    q1[2] = (product[5] >> 60) | (product[6] << 4);
    q1[3] = (product[6] >> 60) | (product[7] << 4);
    q1[4] = product[7] >> 60;

    /* Step 3: q2 = q1 * MU — we only need bits 254 and above, so we compute
     * the full 5x4 product into 9 limbs, but we can use our 4x4 helper for
     * the bottom part and handle q1[4] separately. Since q1[4] is at most 15
     * (4 bits), the extra terms are small. */

    /* q2_low[8] = q1[0..3] * MU[0..3] */
    mul4x4(q2, q1, MU);

    /* Add q1[4] * MU[i] contributions to q2[4..8] */
    {
        unsigned __int128 acc;
        acc = (unsigned __int128)q1[4] * MU[0] + q2[4];
        q2[4] = (uint64_t)acc;
        acc = (unsigned __int128)q1[4] * MU[1] + q2[5] + (uint64_t)(acc >> 64);
        q2[5] = (uint64_t)acc;
        acc = (unsigned __int128)q1[4] * MU[2] + q2[6] + (uint64_t)(acc >> 64);
        q2[6] = (uint64_t)acc;
        acc = (unsigned __int128)q1[4] * MU[3] + q2[7] + (uint64_t)(acc >> 64);
        q2[7] = (uint64_t)acc;
        q2[8] = (uint64_t)(acc >> 64);
    }

    /* Step 4: q3 = q2 >> 254 (shift right by 254 = 3*64 + 62) */
    q3[0] = (q2[3] >> 62) | (q2[4] << 2);
    q3[1] = (q2[4] >> 62) | (q2[5] << 2);
    q3[2] = (q2[5] >> 62) | (q2[6] << 2);
    q3[3] = (q2[6] >> 62) | (q2[7] << 2);

    /* Step 5: r1 = product mod 2^256 (low 4 limbs) */
    r1[0] = product[0];
    r1[1] = product[1];
    r1[2] = product[2];
    r1[3] = product[3];

    /* Step 6: r2 = (q3 * L) mod 2^256 (low 4 limbs) */
    mul_q3_L_low(r2, q3);

    /* Step 7: t = r1 - r2 (mod 2^256) — Barrett approximate reduction of a*b */
    sub4(t, r1, r2);

    /* Step 7b: Constant-time conditional subtract L from t, twice.
     * After Barrett, t may be up to 2L too large. */
    {
        uint64_t borrow = sub4(d, t, L);
        uint64_t mask = ~borrow + 1; /* borrow=1 -> mask=all-1s, borrow=0 -> mask=0 */
        t[0] = d[0] ^ (mask & (t[0] ^ d[0]));
        t[1] = d[1] ^ (mask & (t[1] ^ d[1]));
        t[2] = d[2] ^ (mask & (t[2] ^ d[2]));
        t[3] = d[3] ^ (mask & (t[3] ^ d[3]));
    }
    {
        uint64_t borrow = sub4(d, t, L);
        uint64_t mask = ~borrow + 1;
        t[0] = d[0] ^ (mask & (t[0] ^ d[0]));
        t[1] = d[1] ^ (mask & (t[1] ^ d[1]));
        t[2] = d[2] ^ (mask & (t[2] ^ d[2]));
        t[3] = d[3] ^ (mask & (t[3] ^ d[3]));
    }
    /* Now t = (a * b) mod l, in [0, l) */

    /* Step 8: r = cv - t (mod 2^256) */
    uint64_t borrow = sub4(r, cv, t);

    /* Step 9: Constant-time conditional add L if borrow.
     * Both cv and t are in [0, l), so cv - t is in (-l, l).
     * If borrow, r + L is the correct result. */
    {
        uint64_t mask = ~borrow + 1; /* borrow=1 -> mask=all-1s */
        uint64_t addend[4];
        addend[0] = L[0] & mask;
        addend[1] = L[1] & mask;
        addend[2] = L[2] & mask;
        addend[3] = L[3] & mask;
        add4(r, r, addend);
    }

    std::memcpy(s, r, 32);

    ed25519_secure_erase(av, sizeof(av));
    ed25519_secure_erase(bv, sizeof(bv));
    ed25519_secure_erase(cv, sizeof(cv));
    ed25519_secure_erase(product, sizeof(product));
    ed25519_secure_erase(q1, sizeof(q1));
    ed25519_secure_erase(q2, sizeof(q2));
    ed25519_secure_erase(q3, sizeof(q3));
    ed25519_secure_erase(r1, sizeof(r1));
    ed25519_secure_erase(r2, sizeof(r2));
    ed25519_secure_erase(r, sizeof(r));
    ed25519_secure_erase(t, sizeof(t));
    ed25519_secure_erase(d, sizeof(d));
}

#elif ED25519_HAVE_UMUL128

/*
 * Multiply-accumulate: (c2:c1:c0) += a * b
 * Uses _umul128 for 64x64->128 and _addcarry_u64 for carry propagation.
 */
static inline void mulacc(uint64_t &c0, uint64_t &c1, uint64_t &c2, uint64_t a, uint64_t b)
{
    uint64_t hi;
    uint64_t lo = _umul128(a, b, &hi);
    unsigned char carry = _addcarry_u64(0, c0, lo, &c0);
    carry = _addcarry_u64(carry, c1, hi, &c1);
    _addcarry_u64(carry, c2, 0, &c2);
}

/*
 * 4x4 schoolbook multiply: out[8] = a[4] * b[4]
 * Uses 3-word accumulator (c2:c1:c0) per column.
 */
static void mul4x4(uint64_t out[8], const uint64_t a[4], const uint64_t b[4])
{
    uint64_t c0, c1, c2;

    /* Column 0 */
    c0 = 0;
    c1 = 0;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[0]);
    out[0] = c0;

    /* Column 1 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[1]);
    mulacc(c0, c1, c2, a[1], b[0]);
    out[1] = c0;

    /* Column 2 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[2]);
    mulacc(c0, c1, c2, a[1], b[1]);
    mulacc(c0, c1, c2, a[2], b[0]);
    out[2] = c0;

    /* Column 3 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[0], b[3]);
    mulacc(c0, c1, c2, a[1], b[2]);
    mulacc(c0, c1, c2, a[2], b[1]);
    mulacc(c0, c1, c2, a[3], b[0]);
    out[3] = c0;

    /* Column 4 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[1], b[3]);
    mulacc(c0, c1, c2, a[2], b[2]);
    mulacc(c0, c1, c2, a[3], b[1]);
    out[4] = c0;

    /* Column 5 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[2], b[3]);
    mulacc(c0, c1, c2, a[3], b[2]);
    out[5] = c0;

    /* Column 6 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, a[3], b[3]);
    out[6] = c0;
    out[7] = c1;
}

/*
 * Compute low 4 limbs of q3 * L.
 * Exploits L[2] = 0 and L[3] = 0x1000000000000000 = 2^60.
 */
static void mul_q3_L_low(uint64_t out[4], const uint64_t q3[4])
{
    uint64_t c0, c1, c2;
    uint64_t hi;

    /* Column 0: q3[0]*L[0] */
    out[0] = _umul128(q3[0], L[0], &hi);
    c1 = hi;
    c2 = 0;

    /* Column 1: q3[0]*L[1] + q3[1]*L[0] */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, q3[0], L[1]);
    mulacc(c0, c1, c2, q3[1], L[0]);
    out[1] = c0;

    /* Column 2: q3[1]*L[1] + q3[2]*L[0] (L[2]=0 terms vanish) */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, q3[1], L[1]);
    mulacc(c0, c1, c2, q3[2], L[0]);
    out[2] = c0;

    /* Column 3: q3[2]*L[1] + q3[3]*L[0] + q3[0]*L[3]
     * L[3] = 2^60, so q3[0]*L[3] = q3[0] << 60 */
    c0 = c1;
    c1 = c2;
    c2 = 0;
    mulacc(c0, c1, c2, q3[2], L[1]);
    mulacc(c0, c1, c2, q3[3], L[0]);
    /* Add q3[0] << 60 */
    unsigned char carry = _addcarry_u64(0, c0, q3[0] << 60, &c0);
    _addcarry_u64(carry, c1, q3[0] >> 4, &c1);
    out[3] = c0;
}

void sc_mulsub_x64(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c)
{
    uint64_t av[4], bv[4], cv[4];
    uint64_t product[8];
    uint64_t q1[5], q2[9], q3[4];
    uint64_t r1[4], r2[4], r[4], t[4], d[4];

    std::memcpy(av, a, 32);
    std::memcpy(bv, b, 32);
    std::memcpy(cv, c, 32);

    /* Mask inputs to 253 bits (Barrett requires product < 2^506) */
    av[3] &= UINT64_C(0x1FFFFFFFFFFFFFFF);
    bv[3] &= UINT64_C(0x1FFFFFFFFFFFFFFF);
    cv[3] &= UINT64_C(0x1FFFFFFFFFFFFFFF);

    /* Step 1: Schoolbook multiply product = av * bv (8 limbs) */
    mul4x4(product, av, bv);

    /* Step 2: q1 = product >> 252 (shift right by 252 = 3*64 + 60) */
    q1[0] = (product[3] >> 60) | (product[4] << 4);
    q1[1] = (product[4] >> 60) | (product[5] << 4);
    q1[2] = (product[5] >> 60) | (product[6] << 4);
    q1[3] = (product[6] >> 60) | (product[7] << 4);
    q1[4] = product[7] >> 60;

    /* Step 3: q2 = q1 * MU (5x4 -> 9 limbs)
     * Compute q1[0..3] * MU via mul4x4, then add q1[4] * MU[i] */
    mul4x4(q2, q1, MU);

    /* Add q1[4] * MU[i] to q2[4..8] using carry chain */
    {
        uint64_t hi, lo;
        unsigned char carry;

        lo = _umul128(q1[4], MU[0], &hi);
        carry = _addcarry_u64(0, q2[4], lo, &q2[4]);
        carry = _addcarry_u64(carry, q2[5], hi, &q2[5]);
        carry = _addcarry_u64(carry, q2[6], 0, &q2[6]);
        carry = _addcarry_u64(carry, q2[7], 0, &q2[7]);

        lo = _umul128(q1[4], MU[1], &hi);
        carry = _addcarry_u64(0, q2[5], lo, &q2[5]);
        carry = _addcarry_u64(carry, q2[6], hi, &q2[6]);
        carry = _addcarry_u64(carry, q2[7], 0, &q2[7]);

        lo = _umul128(q1[4], MU[2], &hi);
        carry = _addcarry_u64(0, q2[6], lo, &q2[6]);
        carry = _addcarry_u64(carry, q2[7], hi, &q2[7]);

        lo = _umul128(q1[4], MU[3], &hi);
        carry = _addcarry_u64(0, q2[7], lo, &q2[7]);
        _addcarry_u64(carry, 0, hi, &q2[8]);
    }

    /* Step 4: q3 = q2 >> 254 (shift right by 254 = 3*64 + 62) */
    q3[0] = (q2[3] >> 62) | (q2[4] << 2);
    q3[1] = (q2[4] >> 62) | (q2[5] << 2);
    q3[2] = (q2[5] >> 62) | (q2[6] << 2);
    q3[3] = (q2[6] >> 62) | (q2[7] << 2);

    /* Step 5: r1 = product mod 2^256 (low 4 limbs) */
    r1[0] = product[0];
    r1[1] = product[1];
    r1[2] = product[2];
    r1[3] = product[3];

    /* Step 6: r2 = (q3 * L) mod 2^256 (low 4 limbs) */
    mul_q3_L_low(r2, q3);

    /* Step 7: t = r1 - r2 (mod 2^256) — Barrett approximate reduction of a*b */
    unsigned char borrow = 0;
    borrow = _subborrow_u64(borrow, r1[0], r2[0], &t[0]);
    borrow = _subborrow_u64(borrow, r1[1], r2[1], &t[1]);
    borrow = _subborrow_u64(borrow, r1[2], r2[2], &t[2]);
    _subborrow_u64(borrow, r1[3], r2[3], &t[3]);

    /* Step 7b: Constant-time conditional subtract L from t, twice.
     * After Barrett, t may be up to 2L too large. */
    borrow = 0;
    borrow = _subborrow_u64(borrow, t[0], L[0], &d[0]);
    borrow = _subborrow_u64(borrow, t[1], L[1], &d[1]);
    borrow = _subborrow_u64(borrow, t[2], L[2], &d[2]);
    borrow = _subborrow_u64(borrow, t[3], L[3], &d[3]);
    {
        uint64_t mask = ~(uint64_t)borrow + 1; /* borrow=1 -> mask=all-1s, borrow=0 -> mask=0 */
        t[0] = d[0] ^ (mask & (t[0] ^ d[0]));
        t[1] = d[1] ^ (mask & (t[1] ^ d[1]));
        t[2] = d[2] ^ (mask & (t[2] ^ d[2]));
        t[3] = d[3] ^ (mask & (t[3] ^ d[3]));
    }
    borrow = 0;
    borrow = _subborrow_u64(borrow, t[0], L[0], &d[0]);
    borrow = _subborrow_u64(borrow, t[1], L[1], &d[1]);
    borrow = _subborrow_u64(borrow, t[2], L[2], &d[2]);
    borrow = _subborrow_u64(borrow, t[3], L[3], &d[3]);
    {
        uint64_t mask = ~(uint64_t)borrow + 1;
        t[0] = d[0] ^ (mask & (t[0] ^ d[0]));
        t[1] = d[1] ^ (mask & (t[1] ^ d[1]));
        t[2] = d[2] ^ (mask & (t[2] ^ d[2]));
        t[3] = d[3] ^ (mask & (t[3] ^ d[3]));
    }
    /* Now t = (a * b) mod l, in [0, l) */

    /* Step 8: r = cv - t (mod 2^256) */
    borrow = 0;
    borrow = _subborrow_u64(borrow, cv[0], t[0], &r[0]);
    borrow = _subborrow_u64(borrow, cv[1], t[1], &r[1]);
    borrow = _subborrow_u64(borrow, cv[2], t[2], &r[2]);
    borrow = _subborrow_u64(borrow, cv[3], t[3], &r[3]);

    /* Step 9: Constant-time conditional add L if borrow.
     * Both cv and t are in [0, l), so cv - t is in (-l, l).
     * If borrow, r + L is the correct result. */
    {
        uint64_t mask = ~(uint64_t)borrow + 1; /* borrow=1 -> mask=all-1s */
        unsigned char carry = _addcarry_u64(0, r[0], L[0] & mask, &r[0]);
        carry = _addcarry_u64(carry, r[1], L[1] & mask, &r[1]);
        carry = _addcarry_u64(carry, r[2], L[2] & mask, &r[2]);
        _addcarry_u64(carry, r[3], L[3] & mask, &r[3]);
    }

    std::memcpy(s, r, 32);

    ed25519_secure_erase(av, sizeof(av));
    ed25519_secure_erase(bv, sizeof(bv));
    ed25519_secure_erase(cv, sizeof(cv));
    ed25519_secure_erase(product, sizeof(product));
    ed25519_secure_erase(q1, sizeof(q1));
    ed25519_secure_erase(q2, sizeof(q2));
    ed25519_secure_erase(q3, sizeof(q3));
    ed25519_secure_erase(r1, sizeof(r1));
    ed25519_secure_erase(r2, sizeof(r2));
    ed25519_secure_erase(r, sizeof(r));
    ed25519_secure_erase(t, sizeof(t));
    ed25519_secure_erase(d, sizeof(d));
}

#endif
