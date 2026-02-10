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
 * @file x64/sc_reduce.cpp
 * @brief x64 implementation of scalar reduction modulo l (32-byte and 64-byte).
 *
 * Exploits l = 2^252 + c where c = { 0x5812631a5cf5d3ed, 0x14def9dea2f79cd6 }.
 * 64-byte reduction uses precomputed constants R4..R7 = 2^(64*i) mod l.
 */

#include "ed25519_platform.h"
#include "ed25519_secure_erase.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

/* Group order l as 4 x uint64_t (little-endian) */
static const uint64_t L[4] = {
    UINT64_C(0x5812631a5cf5d3ed),
    UINT64_C(0x14def9dea2f79cd6),
    UINT64_C(0x0000000000000000),
    UINT64_C(0x1000000000000000),
};

/* Low 128 bits of l (the "c" in l = 2^252 + c) */
static const uint64_t C_LO = UINT64_C(0x5812631a5cf5d3ed);
static const uint64_t C_HI = UINT64_C(0x14def9dea2f79cd6);

/* 2^256 mod l */
static const uint64_t R4[4] = {
    UINT64_C(0xd6ec31748d98951d),
    UINT64_C(0xc6ef5bf4737dcf70),
    UINT64_C(0xfffffffffffffffe),
    UINT64_C(0x0fffffffffffffff),
};

/* 2^320 mod l */
static const uint64_t R5[4] = {
    UINT64_C(0x5812631a5cf5d3ed),
    UINT64_C(0x93b8c838d39a5e06),
    UINT64_C(0xb2106215d086329a),
    UINT64_C(0x0ffffffffffffffe),
};

/* 2^384 mod l */
static const uint64_t R6[4] = {
    UINT64_C(0x39822129a02a6271),
    UINT64_C(0xb64a7f435e4fdd95),
    UINT64_C(0x7ed9ce5a30a2c131),
    UINT64_C(0x02106215d086329a),
};

/* 2^448 mod l */
static const uint64_t R7[4] = {
    UINT64_C(0x79daf520a00acb65),
    UINT64_C(0xe24babbe38d1d7a9),
    UINT64_C(0xb399411b7c309a3d),
    UINT64_C(0x0ed9ce5a30a2c131),
};

/*
 * Reduce a 512-bit (64-byte) value modulo l using native 4x64-bit arithmetic.
 *
 * Given input as 8 x uint64_t (little-endian), we compute:
 *   value ≡ sv[0..3] + sv[4]*R4 + sv[5]*R5 + sv[6]*R6 + sv[7]*R7  (mod l)
 * where Ri = 2^(64*i) mod l.
 *
 * After accumulation into a 5-limb result, fold bits above 252 using
 * 2^252 ≡ -c (mod l) where c = {C_LO, C_HI}. Then conditionally subtract l.
 */
static void sc_reduce_64_x64(unsigned char *s)
{
    uint64_t sv[8];
    std::memcpy(sv, s, 64);

    /*
     * Reduce 512-bit input modulo l using precomputed constants.
     * value = sv[0..3] + sv[4]*R4 + sv[5]*R5 + sv[6]*R6 + sv[7]*R7  (mod l)
     *
     * Accumulate into 5-limb result: acc[0..4].
     * Each sv[i] * R[i][j] is a 128-bit product; we accumulate with carries.
     */

    uint64_t acc[5];
    acc[0] = sv[0];
    acc[1] = sv[1];
    acc[2] = sv[2];
    acc[3] = sv[3];
    acc[4] = 0;

#if ED25519_HAVE_INT128
    /* For each upper limb, multiply by its R constant and accumulate */
    const uint64_t *Rtab[4] = {R4, R5, R6, R7};

    for (int i = 0; i < 4; i++)
    {
        uint64_t m = sv[4 + i];
        const uint64_t *R = Rtab[i];
        uint64_t carry = 0;

        for (int j = 0; j < 4; j++)
        {
            unsigned __int128 prod = (unsigned __int128)m * R[j] + acc[j] + carry;
            acc[j] = (uint64_t)prod;
            carry = (uint64_t)(prod >> 64);
        }
        acc[4] += carry;
    }

    /*
     * Fold acc[4] back into acc[0..3] by multiplying by R4 (= 2^256 mod l).
     * Since acc[4] is at most ~66 bits and R4[3] < 2^60, the new acc[4] will be
     * at most ~62 bits. One more fold brings it down to ~58 bits, then we extract
     * the remaining top bits of acc[3] with the simple q = acc[3] >> 60 pattern.
     */
    for (int iter = 0; iter < 2; iter++)
    {
        uint64_t m = acc[4];
        acc[4] = 0;
        uint64_t carry = 0;

        for (int j = 0; j < 4; j++)
        {
            unsigned __int128 prod = (unsigned __int128)m * R4[j] + acc[j] + carry;
            acc[j] = (uint64_t)prod;
            carry = (uint64_t)(prod >> 64);
        }
        acc[4] = carry;
    }

    /*
     * Now acc[4] is small enough that (acc[4] << 4) fits in 64 bits.
     * Extract q = bits 252+ = (acc[4] << 4) | (acc[3] >> 60), at most ~62 bits.
     * Subtract q * c from acc[0..3], same pattern as sc_reduce_32_x64.
     */
    {
        uint64_t q = (acc[4] << 4) | (acc[3] >> 60);
        acc[3] &= UINT64_C(0x0FFFFFFFFFFFFFFF);

        unsigned __int128 prod = (unsigned __int128)q * C_LO;
        uint64_t p0 = (uint64_t)prod;
        uint64_t pc = (uint64_t)(prod >> 64);

        prod = (unsigned __int128)q * C_HI + pc;
        uint64_t p1 = (uint64_t)prod;
        uint64_t p2 = (uint64_t)(prod >> 64);

        /* r = acc - p (borrow chain) */
        __int128 sacc = (__int128)acc[0] - p0;
        acc[0] = (uint64_t)sacc;
        sacc = (__int128)acc[1] - p1 + (int64_t)(sacc >> 64);
        acc[1] = (uint64_t)sacc;
        sacc = (__int128)acc[2] - p2 + (int64_t)(sacc >> 64);
        acc[2] = (uint64_t)sacc;
        sacc = (__int128)acc[3] - 0 + (int64_t)(sacc >> 64);
        acc[3] = (uint64_t)sacc;

        /* If underflowed, add l back */
        uint64_t borrow_mask = (uint64_t)((int64_t)(sacc >> 64));
        unsigned __int128 uacc = (unsigned __int128)acc[0] + (borrow_mask & L[0]);
        acc[0] = (uint64_t)uacc;
        uacc = (unsigned __int128)acc[1] + (borrow_mask & L[1]) + (uint64_t)(uacc >> 64);
        acc[1] = (uint64_t)uacc;
        uacc = (unsigned __int128)acc[2] + (borrow_mask & L[2]) + (uint64_t)(uacc >> 64);
        acc[2] = (uint64_t)uacc;
        uacc = (unsigned __int128)acc[3] + (borrow_mask & L[3]) + (uint64_t)(uacc >> 64);
        acc[3] = (uint64_t)uacc;
    }

    /*
     * One more fold may be needed: q*c subtraction can leave up to 4 bits above 252.
     * This mirrors the second fold pass in sc_reduce_32_x64 style.
     */
    {
        uint64_t q = acc[3] >> 60;
        acc[3] &= UINT64_C(0x0FFFFFFFFFFFFFFF);

        unsigned __int128 prod = (unsigned __int128)q * C_LO;
        uint64_t p0 = (uint64_t)prod;
        uint64_t pc = (uint64_t)(prod >> 64);

        prod = (unsigned __int128)q * C_HI + pc;
        uint64_t p1 = (uint64_t)prod;
        uint64_t p2 = (uint64_t)(prod >> 64);

        __int128 sacc = (__int128)acc[0] - p0;
        acc[0] = (uint64_t)sacc;
        sacc = (__int128)acc[1] - p1 + (int64_t)(sacc >> 64);
        acc[1] = (uint64_t)sacc;
        sacc = (__int128)acc[2] - p2 + (int64_t)(sacc >> 64);
        acc[2] = (uint64_t)sacc;
        sacc = (__int128)acc[3] - 0 + (int64_t)(sacc >> 64);
        acc[3] = (uint64_t)sacc;

        uint64_t borrow_mask = (uint64_t)((int64_t)(sacc >> 64));
        unsigned __int128 uacc = (unsigned __int128)acc[0] + (borrow_mask & L[0]);
        acc[0] = (uint64_t)uacc;
        uacc = (unsigned __int128)acc[1] + (borrow_mask & L[1]) + (uint64_t)(uacc >> 64);
        acc[1] = (uint64_t)uacc;
        uacc = (unsigned __int128)acc[2] + (borrow_mask & L[2]) + (uint64_t)(uacc >> 64);
        acc[2] = (uint64_t)uacc;
        uacc = (unsigned __int128)acc[3] + (borrow_mask & L[3]) + (uint64_t)(uacc >> 64);
        acc[3] = (uint64_t)uacc;
    }

    /* Final conditional subtract: if acc >= l, compute acc - l */
    {
        __int128 sacc = (__int128)acc[0] - L[0];
        uint64_t d0 = (uint64_t)sacc;
        sacc = (__int128)acc[1] - L[1] + (int64_t)(sacc >> 64);
        uint64_t d1 = (uint64_t)sacc;
        sacc = (__int128)acc[2] - L[2] + (int64_t)(sacc >> 64);
        uint64_t d2 = (uint64_t)sacc;
        sacc = (__int128)acc[3] - L[3] + (int64_t)(sacc >> 64);
        uint64_t d3 = (uint64_t)sacc;

        uint64_t mask = (uint64_t)((int64_t)(sacc >> 64));
        acc[0] = d0 ^ (mask & (acc[0] ^ d0));
        acc[1] = d1 ^ (mask & (acc[1] ^ d1));
        acc[2] = d2 ^ (mask & (acc[2] ^ d2));
        acc[3] = d3 ^ (mask & (acc[3] ^ d3));
    }

#elif ED25519_HAVE_UMUL128
    const uint64_t *Rtab[4] = {R4, R5, R6, R7};

    for (int i = 0; i < 4; i++)
    {
        uint64_t m = sv[4 + i];
        const uint64_t *R = Rtab[i];
        uint64_t carry = 0;

        for (int j = 0; j < 4; j++)
        {
            uint64_t hi;
            uint64_t lo = _umul128(m, R[j], &hi);

            unsigned char c = 0;
            c = _addcarry_u64(0, lo, acc[j], &lo);
            _addcarry_u64(c, hi, 0, &hi);

            c = _addcarry_u64(0, lo, carry, &acc[j]);
            carry = hi + c;
        }
        acc[4] += carry;
    }

    /* Fold acc[4] back into acc[0..3] by multiplying by R4 (= 2^256 mod l) */
    for (int iter = 0; iter < 2; iter++)
    {
        uint64_t m = acc[4];
        acc[4] = 0;
        uint64_t carry = 0;

        for (int j = 0; j < 4; j++)
        {
            uint64_t hi;
            uint64_t lo = _umul128(m, R4[j], &hi);

            unsigned char c = 0;
            c = _addcarry_u64(0, lo, acc[j], &lo);
            _addcarry_u64(c, hi, 0, &hi);

            c = _addcarry_u64(0, lo, carry, &acc[j]);
            carry = hi + c;
        }
        acc[4] = carry;
    }

    /* Extract q = bits 252+, subtract q * c */
    {
        uint64_t q = (acc[4] << 4) | (acc[3] >> 60);
        acc[3] &= UINT64_C(0x0FFFFFFFFFFFFFFF);

        uint64_t p0_hi;
        uint64_t p0 = _umul128(q, C_LO, &p0_hi);

        uint64_t p1_hi;
        uint64_t p1 = _umul128(q, C_HI, &p1_hi);

        unsigned char c = 0;
        c = _addcarry_u64(0, p1, p0_hi, &p1);
        uint64_t p2 = p1_hi + c;

        unsigned char borrow = 0;
        borrow = _subborrow_u64(borrow, acc[0], p0, &acc[0]);
        borrow = _subborrow_u64(borrow, acc[1], p1, &acc[1]);
        borrow = _subborrow_u64(borrow, acc[2], p2, &acc[2]);
        borrow = _subborrow_u64(borrow, acc[3], 0, &acc[3]);

        uint64_t borrow_mask = ~(uint64_t)borrow + 1;

        c = 0;
        c = _addcarry_u64(c, acc[0], borrow_mask & L[0], &acc[0]);
        c = _addcarry_u64(c, acc[1], borrow_mask & L[1], &acc[1]);
        c = _addcarry_u64(c, acc[2], borrow_mask & L[2], &acc[2]);
        c = _addcarry_u64(c, acc[3], borrow_mask & L[3], &acc[3]);
    }

    /* Second fold: handle remaining top bits of acc[3] */
    {
        uint64_t q = acc[3] >> 60;
        acc[3] &= UINT64_C(0x0FFFFFFFFFFFFFFF);

        uint64_t p0_hi;
        uint64_t p0 = _umul128(q, C_LO, &p0_hi);

        uint64_t p1_hi;
        uint64_t p1 = _umul128(q, C_HI, &p1_hi);

        unsigned char c = 0;
        c = _addcarry_u64(0, p1, p0_hi, &p1);
        uint64_t p2 = p1_hi + c;

        unsigned char borrow = 0;
        borrow = _subborrow_u64(borrow, acc[0], p0, &acc[0]);
        borrow = _subborrow_u64(borrow, acc[1], p1, &acc[1]);
        borrow = _subborrow_u64(borrow, acc[2], p2, &acc[2]);
        borrow = _subborrow_u64(borrow, acc[3], 0, &acc[3]);

        uint64_t borrow_mask = ~(uint64_t)borrow + 1;

        c = 0;
        c = _addcarry_u64(c, acc[0], borrow_mask & L[0], &acc[0]);
        c = _addcarry_u64(c, acc[1], borrow_mask & L[1], &acc[1]);
        c = _addcarry_u64(c, acc[2], borrow_mask & L[2], &acc[2]);
        c = _addcarry_u64(c, acc[3], borrow_mask & L[3], &acc[3]);
    }

    /* Final conditional subtract */
    {
        uint64_t d[4];
        unsigned char borrow = 0;
        borrow = _subborrow_u64(borrow, acc[0], L[0], &d[0]);
        borrow = _subborrow_u64(borrow, acc[1], L[1], &d[1]);
        borrow = _subborrow_u64(borrow, acc[2], L[2], &d[2]);
        borrow = _subborrow_u64(borrow, acc[3], L[3], &d[3]);

        uint64_t mask = ~(uint64_t)borrow + 1;
        acc[0] = d[0] ^ (mask & (acc[0] ^ d[0]));
        acc[1] = d[1] ^ (mask & (acc[1] ^ d[1]));
        acc[2] = d[2] ^ (mask & (acc[2] ^ d[2]));
        acc[3] = d[3] ^ (mask & (acc[3] ^ d[3]));
    }
#endif

    std::memcpy(s, acc, 32);

    ed25519_secure_erase(sv, sizeof(sv));
    ed25519_secure_erase(acc, sizeof(acc));
}

static void sc_reduce_32_x64(unsigned char *s)
{
    uint64_t sv[4];
    std::memcpy(sv, s, 32);

    /* q = top 4 bits of sv[3] (value in [0, 15]) */
    uint64_t q = sv[3] >> 60;
    sv[3] &= UINT64_C(0x0FFFFFFFFFFFFFFF); /* now sv < 2^252 */

    /*
     * product = q * c, where c is 128-bit. q is at most 15, so product fits in 132 bits (3 limbs).
     * We need to compute r = sv - product, then conditionally add l if r underflowed.
     */

#if ED25519_HAVE_INT128
    unsigned __int128 prod = (unsigned __int128)q * C_LO;
    uint64_t p0 = (uint64_t)prod;
    uint64_t carry = (uint64_t)(prod >> 64);

    prod = (unsigned __int128)q * C_HI + carry;
    uint64_t p1 = (uint64_t)prod;
    uint64_t p2 = (uint64_t)(prod >> 64);

    /* r = sv - product (borrow chain over 4 limbs) */
    __int128 sacc = (__int128)sv[0] - p0;
    uint64_t r0 = (uint64_t)sacc;
    sacc = (__int128)sv[1] - p1 + (int64_t)(sacc >> 64);
    uint64_t r1 = (uint64_t)sacc;
    sacc = (__int128)sv[2] - p2 + (int64_t)(sacc >> 64);
    uint64_t r2 = (uint64_t)sacc;
    sacc = (__int128)sv[3] - 0 + (int64_t)(sacc >> 64);
    uint64_t r3 = (uint64_t)sacc;

    /* borrow_mask: all-ones if r underflowed, 0 otherwise */
    uint64_t borrow_mask = (uint64_t)((int64_t)(sacc >> 64));

    /* Conditional add-back: r += l & borrow_mask */
    unsigned __int128 acc = (unsigned __int128)r0 + (borrow_mask & L[0]);
    r0 = (uint64_t)acc;
    acc = (unsigned __int128)r1 + (borrow_mask & L[1]) + (uint64_t)(acc >> 64);
    r1 = (uint64_t)acc;
    acc = (unsigned __int128)r2 + (borrow_mask & L[2]) + (uint64_t)(acc >> 64);
    r2 = (uint64_t)acc;
    acc = (unsigned __int128)r3 + (borrow_mask & L[3]) + (uint64_t)(acc >> 64);
    r3 = (uint64_t)acc;
#elif ED25519_HAVE_UMUL128
    uint64_t p0_hi;
    uint64_t p0 = _umul128(q, C_LO, &p0_hi);

    uint64_t p1_hi;
    uint64_t p1 = _umul128(q, C_HI, &p1_hi);

    /* Add carry from low product into high product */
    unsigned char c = 0;
    c = _addcarry_u64(0, p1, p0_hi, &p1);
    uint64_t p2 = p1_hi + c;

    /* r = sv - product (borrow chain) */
    uint64_t r0, r1, r2, r3;
    unsigned char borrow = 0;
    borrow = _subborrow_u64(borrow, sv[0], p0, &r0);
    borrow = _subborrow_u64(borrow, sv[1], p1, &r1);
    borrow = _subborrow_u64(borrow, sv[2], p2, &r2);
    borrow = _subborrow_u64(borrow, sv[3], 0, &r3);

    /* borrow_mask: all-ones if underflowed, 0 otherwise */
    uint64_t borrow_mask = ~(uint64_t)borrow + 1;

    /* Conditional add-back: r += l & borrow_mask */
    c = 0;
    c = _addcarry_u64(c, r0, borrow_mask & L[0], &r0);
    c = _addcarry_u64(c, r1, borrow_mask & L[1], &r1);
    c = _addcarry_u64(c, r2, borrow_mask & L[2], &r2);
    c = _addcarry_u64(c, r3, borrow_mask & L[3], &r3);
#endif

    sv[0] = r0;
    sv[1] = r1;
    sv[2] = r2;
    sv[3] = r3;
    std::memcpy(s, sv, 32);

    ed25519_secure_erase(sv, sizeof(sv));
    ed25519_secure_erase(&q, sizeof(q));
    ed25519_secure_erase(&r0, sizeof(r0));
    ed25519_secure_erase(&r1, sizeof(r1));
    ed25519_secure_erase(&r2, sizeof(r2));
    ed25519_secure_erase(&r3, sizeof(r3));
    ed25519_secure_erase(&borrow_mask, sizeof(borrow_mask));
}

void sc_reduce_x64(unsigned char *s, size_t len)
{
    if (len == 64)
        sc_reduce_64_x64(s);
    else
        sc_reduce_32_x64(s);
}
