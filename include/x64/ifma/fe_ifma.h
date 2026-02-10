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
 * @file fe_ifma.h
 * @brief AVX-512 IFMA field element multiplication and squaring.
 *
 * This is the field arithmetic layer for the single-op IFMA scalarmult
 * backend (the 4 TUs in src/x64/ifma/). It replaces the scalar fe51_mul
 * and fe51_sq with IFMA hardware multiply-accumulate, which is faster
 * than MSVC's uint128 emulation path (15-18% speedup on scalar multiplication).
 *
 * The algorithm packs g's 5 limbs into 5 adjacent lanes of a single __m512i,
 * then broadcasts each limb of f and multiplies against a shifted view of g
 * using _mm512_alignr_epi64. This produces the schoolbook product across
 * the 8 lanes of the ZMM register (only 5 lanes are used, the rest are zero-
 * padded). The 9-limb result is split into lo (bits 0-51) and hi (bits 52-103)
 * by the vpmadd52lo/vpmadd52hi instruction pair, then folded and carried.
 *
 * This is a different layout from fe51x8_ifma.h (the 8-way batch variant),
 * which packs 8 independent field elements across lanes instead. Here, one
 * field element is spread across 5 lanes of one register; there, 8 copies
 * of one limb share one register.
 *
 * There are two variants of each operation:
 *   - Normalizing (fe_mul_ifma, fe_sq_ifma): carry-propagates both inputs
 *     to ≤51 bits before the IFMA rounds. Safe for any input.
 *   - Non-normalizing (fe_mul_ifma_nn, fe_sq_ifma_n): skips input carry-
 *     propagation, relying on the caller to ensure limbs are ≤52 bits. This
 *     saves ~56 ops per multiply. Used when inputs are known to be within
 *     bounds (IFMA outputs ≤51 bits, fe_add of two ≤51-bit ≤52, fe_sub ≤52).
 */

#ifndef ED25519_X64_IFMA_FE_IFMA_H
#define ED25519_X64_IFMA_FE_IFMA_H

#include "ed25519_platform.h"
#include "fe.h"
#include "x64/fe51.h"

#include <immintrin.h>

#if defined(_MSC_VER)
#define FE_IFMA_FORCE_INLINE __forceinline
#else
#define FE_IFMA_FORCE_INLINE inline __attribute__((always_inline))
#endif

/**
 * @brief Weak normalization: carry-propagate to ensure all limbs ≤52 bits.
 *
 * This is a single-pass carry chain (~8 scalar ops) used to fix the one
 * problematic fe_add output in ge_add/ge_sub/ge_madd/ge_msub where adding
 * a ≤52-bit value to a ≤51-bit value can produce ≤53-bit limbs.
 * After this, all limbs are ≤51 bits (safe for IFMA's 52-bit input window).
 */
static FE_IFMA_FORCE_INLINE void fe_normalize_weak(fe h)
{
    uint64_t c;
    c = h[0] >> 51;
    h[1] += c;
    h[0] &= FE51_MASK;
    c = h[1] >> 51;
    h[2] += c;
    h[1] &= FE51_MASK;
    c = h[2] >> 51;
    h[3] += c;
    h[2] &= FE51_MASK;
    c = h[3] >> 51;
    h[4] += c;
    h[3] &= FE51_MASK;
    c = h[4] >> 51;
    h[0] += c * 19;
    h[4] &= FE51_MASK;
    c = h[0] >> 51;
    h[1] += c;
    h[0] &= FE51_MASK;
}

/**
 * @brief Core IFMA multiply: h = f * g (mod 2^255-19), inputs already ≤52 bits.
 *
 * This is the raw IFMA multiply without input normalization. Callers must
 * ensure both inputs have limbs ≤52 bits (which is true for IFMA mul/sq
 * outputs, fe_add of two ≤51-bit values, and fe_sub outputs).
 */
static FE_IFMA_FORCE_INLINE void fe_mul_ifma_core(fe h, const uint64_t *fn, const uint64_t *gn)
{
    const __m512i zeros = _mm512_setzero_si512();

    const __m512i g_vec = _mm512_set_epi64(
        0, 0, 0, (long long)gn[4], (long long)gn[3], (long long)gn[2], (long long)gn[1], (long long)gn[0]);

    __m512i lo_acc = zeros;
    __m512i hi_acc = zeros;

    __m512i f_bcast = _mm512_set1_epi64((long long)fn[0]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_vec);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_vec);

    __m512i g_shifted = _mm512_alignr_epi64(g_vec, zeros, 7);
    f_bcast = _mm512_set1_epi64((long long)fn[1]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    g_shifted = _mm512_alignr_epi64(g_vec, zeros, 6);
    f_bcast = _mm512_set1_epi64((long long)fn[2]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    g_shifted = _mm512_alignr_epi64(g_vec, zeros, 5);
    f_bcast = _mm512_set1_epi64((long long)fn[3]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    g_shifted = _mm512_alignr_epi64(g_vec, zeros, 4);
    f_bcast = _mm512_set1_epi64((long long)fn[4]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    alignas(64) uint64_t lo[8], hi[8];
    _mm512_store_si512(lo, lo_acc);
    _mm512_store_si512(hi, hi_acc);

    const __m512i f4_vec = _mm512_set1_epi64((long long)fn[4]);
    const __m512i g4_vec = _mm512_set1_epi64((long long)gn[4]);
    const __m512i p8_lo_vec = _mm512_madd52lo_epu64(zeros, f4_vec, g4_vec);
    const __m512i p8_hi_vec = _mm512_madd52hi_epu64(zeros, f4_vec, g4_vec);
    const uint64_t p8_lo = (uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(p8_lo_vec));
    const uint64_t p8_hi = (uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(p8_hi_vec));

    const uint64_t r_lo0 = lo[0] + 19 * lo[5];
    const uint64_t r_hi0 = hi[0] + 19 * hi[5];
    const uint64_t r_lo1 = lo[1] + 19 * lo[6];
    const uint64_t r_hi1 = hi[1] + 19 * hi[6];
    const uint64_t r_lo2 = lo[2] + 19 * lo[7];
    const uint64_t r_hi2 = hi[2] + 19 * hi[7];
    const uint64_t r_lo3 = lo[3] + 19 * p8_lo;
    const uint64_t r_hi3 = hi[3] + 19 * p8_hi;

    uint64_t carry, val;

    val = r_lo0;
    h[0] = val & FE51_MASK;
    carry = (val >> 51) + r_hi0 * 2;

    val = r_lo1 + carry;
    h[1] = val & FE51_MASK;
    carry = (val >> 51) + r_hi1 * 2;

    val = r_lo2 + carry;
    h[2] = val & FE51_MASK;
    carry = (val >> 51) + r_hi2 * 2;

    val = r_lo3 + carry;
    h[3] = val & FE51_MASK;
    carry = (val >> 51) + r_hi3 * 2;

    val = lo[4] + carry;
    h[4] = val & FE51_MASK;
    carry = (val >> 51) + hi[4] * 2;

    h[0] += carry * 19;
    carry = h[0] >> 51;
    h[0] &= FE51_MASK;
    h[1] += carry;
}

/**
 * @brief IFMA field element multiplication: h = f * g (mod 2^255-19).
 *
 * Uses 5 IFMA rounds to compute the schoolbook product across 8 ZMM lanes,
 * then folds upper limbs with factor 19 and carry-propagates.
 * Normalizes both inputs to ≤51 bits before the IFMA rounds.
 */
static FE_IFMA_FORCE_INLINE void fe_mul_ifma(fe h, const fe f, const fe g)
{
    // IFMA instructions only use the low 52 bits of each source element.
    // After fe_add (which doesn't carry-propagate), limbs can exceed 52 bits.
    // Normalize both inputs to ≤51 bits before the IFMA rounds.
    uint64_t fn[5], gn[5], c;

    fn[0] = f[0];
    c = fn[0] >> 51;
    fn[0] &= FE51_MASK;
    fn[1] = f[1] + c;
    c = fn[1] >> 51;
    fn[1] &= FE51_MASK;
    fn[2] = f[2] + c;
    c = fn[2] >> 51;
    fn[2] &= FE51_MASK;
    fn[3] = f[3] + c;
    c = fn[3] >> 51;
    fn[3] &= FE51_MASK;
    fn[4] = f[4] + c;
    c = fn[4] >> 51;
    fn[4] &= FE51_MASK;
    fn[0] += c * 19;
    c = fn[0] >> 51;
    fn[0] &= FE51_MASK;
    fn[1] += c;

    gn[0] = g[0];
    c = gn[0] >> 51;
    gn[0] &= FE51_MASK;
    gn[1] = g[1] + c;
    c = gn[1] >> 51;
    gn[1] &= FE51_MASK;
    gn[2] = g[2] + c;
    c = gn[2] >> 51;
    gn[2] &= FE51_MASK;
    gn[3] = g[3] + c;
    c = gn[3] >> 51;
    gn[3] &= FE51_MASK;
    gn[4] = g[4] + c;
    c = gn[4] >> 51;
    gn[4] &= FE51_MASK;
    gn[0] += c * 19;
    c = gn[0] >> 51;
    gn[0] &= FE51_MASK;
    gn[1] += c;

    fe_mul_ifma_core(h, fn, gn);
}

/**
 * @brief IFMA multiply with no input normalization: h = f * g (mod 2^255-19).
 *
 * Both inputs must already have limbs ≤52 bits. This is the case when inputs
 * come from IFMA mul/sq outputs (≤51 bits), fe_add of two ≤51-bit values
 * (≤52 bits), or fe_sub outputs (≤52 bits after bias).
 */
static FE_IFMA_FORCE_INLINE void fe_mul_ifma_nn(fe h, const fe f, const fe g)
{
    fe_mul_ifma_core(h, f, g);
}

/**
 * @brief Core IFMA squaring: h = f^2 (mod 2^255-19), input already ≤52 bits.
 */
static FE_IFMA_FORCE_INLINE void fe_sq_ifma_core(fe h, const uint64_t *fn)
{
    fe_mul_ifma_core(h, fn, fn);
}

/**
 * @brief IFMA squaring with no input normalization: h = f^2 (mod 2^255-19).
 *
 * Input must have limbs ≤52 bits.
 */
static FE_IFMA_FORCE_INLINE void fe_sq_ifma_n(fe h, const fe f)
{
    fe_sq_ifma_core(h, f);
}

/**
 * @brief IFMA field element squaring: h = f^2 (mod 2^255-19).
 *
 * Uses the same row-based IFMA approach as fe_mul_ifma with f=g.
 */
static FE_IFMA_FORCE_INLINE void fe_sq_ifma(fe h, const fe f)
{
    fe_mul_ifma(h, f, f);
}

/**
 * @brief Core IFMA squared-and-doubled: h = 2 * f^2 (mod 2^255-19).
 *
 * Input fn must already have limbs ≤52 bits.
 */
static FE_IFMA_FORCE_INLINE void fe_sq2_ifma_core(fe h, const uint64_t *fn)
{
    const __m512i zeros = _mm512_setzero_si512();

    const __m512i g_vec = _mm512_set_epi64(
        0, 0, 0, (long long)fn[4], (long long)fn[3], (long long)fn[2], (long long)fn[1], (long long)fn[0]);

    __m512i lo_acc = zeros;
    __m512i hi_acc = zeros;

    __m512i f_bcast = _mm512_set1_epi64((long long)fn[0]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_vec);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_vec);

    __m512i g_shifted = _mm512_alignr_epi64(g_vec, zeros, 7);
    f_bcast = _mm512_set1_epi64((long long)fn[1]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    g_shifted = _mm512_alignr_epi64(g_vec, zeros, 6);
    f_bcast = _mm512_set1_epi64((long long)fn[2]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    g_shifted = _mm512_alignr_epi64(g_vec, zeros, 5);
    f_bcast = _mm512_set1_epi64((long long)fn[3]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    g_shifted = _mm512_alignr_epi64(g_vec, zeros, 4);
    f_bcast = _mm512_set1_epi64((long long)fn[4]);
    lo_acc = _mm512_madd52lo_epu64(lo_acc, f_bcast, g_shifted);
    hi_acc = _mm512_madd52hi_epu64(hi_acc, f_bcast, g_shifted);

    lo_acc = _mm512_add_epi64(lo_acc, lo_acc);
    hi_acc = _mm512_add_epi64(hi_acc, hi_acc);

    alignas(64) uint64_t lo[8], hi[8];
    _mm512_store_si512(lo, lo_acc);
    _mm512_store_si512(hi, hi_acc);

    const __m512i f4_vec = _mm512_set1_epi64((long long)fn[4]);
    const __m512i p8_lo_vec = _mm512_madd52lo_epu64(zeros, f4_vec, f4_vec);
    const __m512i p8_hi_vec = _mm512_madd52hi_epu64(zeros, f4_vec, f4_vec);
    const uint64_t p8_lo = 2 * (uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(p8_lo_vec));
    const uint64_t p8_hi = 2 * (uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(p8_hi_vec));

    const uint64_t r_lo0 = lo[0] + 19 * lo[5];
    const uint64_t r_hi0 = hi[0] + 19 * hi[5];
    const uint64_t r_lo1 = lo[1] + 19 * lo[6];
    const uint64_t r_hi1 = hi[1] + 19 * hi[6];
    const uint64_t r_lo2 = lo[2] + 19 * lo[7];
    const uint64_t r_hi2 = hi[2] + 19 * hi[7];
    const uint64_t r_lo3 = lo[3] + 19 * p8_lo;
    const uint64_t r_hi3 = hi[3] + 19 * p8_hi;

    uint64_t carry, val;

    val = r_lo0;
    h[0] = val & FE51_MASK;
    carry = (val >> 51) + r_hi0 * 2;

    val = r_lo1 + carry;
    h[1] = val & FE51_MASK;
    carry = (val >> 51) + r_hi1 * 2;

    val = r_lo2 + carry;
    h[2] = val & FE51_MASK;
    carry = (val >> 51) + r_hi2 * 2;

    val = r_lo3 + carry;
    h[3] = val & FE51_MASK;
    carry = (val >> 51) + r_hi3 * 2;

    val = lo[4] + carry;
    h[4] = val & FE51_MASK;
    carry = (val >> 51) + hi[4] * 2;

    h[0] += carry * 19;
    carry = h[0] >> 51;
    h[0] &= FE51_MASK;
    h[1] += carry;
}

/**
 * @brief IFMA field element squared-and-doubled: h = 2 * f^2 (mod 2^255-19).
 *
 * Normalizes input, then computes square-and-double using IFMA.
 */
static FE_IFMA_FORCE_INLINE void fe_sq2_ifma(fe h, const fe f)
{
    uint64_t fn[5], c;
    fn[0] = f[0];
    c = fn[0] >> 51;
    fn[0] &= FE51_MASK;
    fn[1] = f[1] + c;
    c = fn[1] >> 51;
    fn[1] &= FE51_MASK;
    fn[2] = f[2] + c;
    c = fn[2] >> 51;
    fn[2] &= FE51_MASK;
    fn[3] = f[3] + c;
    c = fn[3] >> 51;
    fn[3] &= FE51_MASK;
    fn[4] = f[4] + c;
    c = fn[4] >> 51;
    fn[4] &= FE51_MASK;
    fn[0] += c * 19;
    c = fn[0] >> 51;
    fn[0] &= FE51_MASK;
    fn[1] += c;

    fe_sq2_ifma_core(h, fn);
}

/**
 * @brief IFMA squared-and-doubled with no input normalization: h = 2 * f^2 (mod 2^255-19).
 *
 * Input must have limbs ≤52 bits.
 */
static FE_IFMA_FORCE_INLINE void fe_sq2_ifma_n(fe h, const fe f)
{
    fe_sq2_ifma_core(h, f);
}

/**
 * @brief IFMA repeated squaring: h = f^(2^n) (mod 2^255-19).
 */
static FE_IFMA_FORCE_INLINE void fe_sqn_ifma(fe h, const fe f, int n)
{
    fe_sq_ifma(h, f);
    for (int i = 1; i < n; i++)
        fe_sq_ifma(h, h);
}

/**
 * @brief IFMA repeated squaring with no input normalization: h = f^(2^n) (mod 2^255-19).
 *
 * Input must have limbs ≤52 bits. First sq uses no-norm, subsequent use
 * no-norm since IFMA output is ≤51 bits.
 */
static FE_IFMA_FORCE_INLINE void fe_sqn_ifma_n(fe h, const fe f, int n)
{
    fe_sq_ifma_n(h, f);
    for (int i = 1; i < n; i++)
        fe_sq_ifma_n(h, h);
}

#endif // ED25519_X64_IFMA_FE_IFMA_H
