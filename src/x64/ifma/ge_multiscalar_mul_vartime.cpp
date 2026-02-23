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
 * @file ifma/ge_multiscalar_mul_vartime.cpp
 * @brief AVX-512 IFMA multi-scalar multiplication: 8-way Straus + IFMA Pippenger.
 *
 * Straus (n<=32): splits n points across groups of 8, processing each group
 * in parallel using fe51x8 field arithmetic. Each group has its own 8-way
 * precomputed table, and the accumulator holds 8 partial sums that are
 * combined at the end.
 *
 * Pippenger (n>32): uses inline IFMA single-point ge operations for a ~20-30%
 * speedup from IFMA field arithmetic over the x64 baseline.
 */

#include "ge_multiscalar_mul_vartime.h"

#include "fe_add.h"
#include "fe_copy.h"
#include "fe_neg.h"
#include "fe_sub.h"
#include "ge_add.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p2_0.h"
#include "ge_p2_dbl.h"
#include "ge_p3_0.h"
#include "ge_p3_to_cached.h"
#include "ge_p3_to_p2.h"
#include "ge_scalarmult_base_ct.h"
#include "ge_sub.h"
#include "x64/ifma/fe_ifma_chain.h"
#include "x64/ifma/ge_batch_ifma.h"

#include <cstring>
#include <vector>

// ============================================================================
// Signed digit encoding (same as x64 baseline)
// ============================================================================

static void encode_signed_w4(signed char *digits, const unsigned char *scalar)
{
    int carry = 0;
    for (int i = 0; i < 31; i++)
    {
        carry += scalar[i];
        int carry2 = (carry + 8) >> 4;
        digits[2 * i] = static_cast<signed char>(carry - (carry2 << 4));
        carry = (carry2 + 8) >> 4;
        digits[2 * i + 1] = static_cast<signed char>(carry2 - (carry << 4));
    }
    carry += scalar[31];
    int carry2 = (carry + 8) >> 4;
    digits[62] = static_cast<signed char>(carry - (carry2 << 4));
    digits[63] = static_cast<signed char>(carry2);
}

static int encode_signed_wbit(signed char *digits, const unsigned char *scalar, int w)
{
    const int half = 1 << (w - 1);
    const int mask = (1 << w) - 1;
    const int num_digits = (256 + w - 1) / w;

    int carry = 0;
    for (int i = 0; i < num_digits; i++)
    {
        int bit_pos = i * w;
        int byte_pos = bit_pos / 8;
        int bit_off = bit_pos % 8;

        int raw = 0;
        if (byte_pos < 32)
            raw = scalar[byte_pos] >> bit_off;
        if (byte_pos + 1 < 32 && bit_off + w > 8)
            raw |= static_cast<int>(scalar[byte_pos + 1]) << (8 - bit_off);
        if (byte_pos + 2 < 32 && bit_off + w > 16)
            raw |= static_cast<int>(scalar[byte_pos + 2]) << (16 - bit_off);

        int val = (raw & mask) + carry;
        carry = val >> w;
        val &= mask;

        if (val >= half)
        {
            val -= (1 << w);
            carry = 1;
        }

        digits[i] = static_cast<signed char>(val);
    }

    return num_digits;
}

// ============================================================================
// IFMA 8-way Straus — used for n <= 32
// ============================================================================

// d2 = 2*d constant in fe51
static const fe msm_ifma_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

static void msm_straus_ifma(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    // Broadcast d2 constant
    fe51x8 d2_8x;
    fe51x8_broadcast_d2(&d2_8x, msm_ifma_d2_fe51);

    // Encode all scalars into signed 4-bit digits
    std::vector<signed char> all_digits(n * 64);
    for (size_t i = 0; i < n; i++)
        encode_signed_w4(all_digits.data() + i * 64, scalars + i * 32);

    const size_t num_groups = (n + 7) / 8;

    // Precompute tables: one 8-way table per group of 8 points
    // tables[g][j] = (j+1) * P for each of the 8 points in group g
    std::vector<ge_cached_8x> tables(num_groups * 8);

    for (size_t g = 0; g < num_groups; g++)
    {
        size_t base = g * 8;
        size_t group_n = (n - base < 8) ? (n - base) : 8;

        // Pack points into 8-way
        ge_p3_8x P8x;
        ge_p3_0_8x(&P8x);
        for (size_t k = 0; k < group_n; k++)
            ge_p3_8x_insert(&P8x, &points[base + k], (int)k);

        // Build table: T[0] = 1*P, T[1] = 2*P, ..., T[7] = 8*P
        ge_cached_8x *T = tables.data() + g * 8;
        ge_p3_to_cached_8x(&T[0], &P8x, &d2_8x);
        for (int j = 0; j < 7; j++)
        {
            ge_p1p1_8x t;
            ge_p3_8x u;
            ge_add_8x(&t, &P8x, &T[j]);
            ge_p1p1_to_p3_8x(&u, &t);
            ge_p3_to_cached_8x(&T[j + 1], &u, &d2_8x);
        }
    }

    // Main loop: accumulator holds 8 partial sums
    ge_p3_8x acc;
    ge_p3_0_8x(&acc);
    ge_p2_8x r;
    ge_p2_0_8x(&r);

    for (int d = 63; d >= 0; d--)
    {
        // 4 doublings
        ge_p1p1_8x t;
        ge_p2_dbl_8x(&t, &r);
        ge_p1p1_to_p2_8x(&r, &t);
        ge_p2_dbl_8x(&t, &r);
        ge_p1p1_to_p2_8x(&r, &t);
        ge_p2_dbl_8x(&t, &r);
        ge_p1p1_to_p2_8x(&r, &t);
        ge_p2_dbl_8x(&t, &r);
        ge_p1p1_to_p3_8x(&acc, &t);

        // Process each group of 8 points
        for (size_t g = 0; g < num_groups; g++)
        {
            size_t base = g * 8;
            size_t group_n = (n - base < 8) ? (n - base) : 8;

            // Load per-lane digits from the scalars in this group
            long long lane_digits[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            for (size_t k = 0; k < group_n; k++)
                lane_digits[k] = (long long)all_digits[(base + k) * 64 + d];

            __m512i digits = _mm512_loadu_si512((const __m512i *)lane_digits);

            // Check if all digits are zero (skip entire group)
            __mmask8 nonzero_mask = _mm512_cmpneq_epi64_mask(digits, _mm512_setzero_si512());
            if (nonzero_mask == 0)
                continue;

            // Compute abs_digits and neg_mask
            const __m512i zero = _mm512_setzero_si512();
            __mmask8 neg_mask = _mm512_cmpgt_epi64_mask(zero, digits);
            __m512i sign = _mm512_srai_epi64(digits, 63);
            __m512i abs_digits = _mm512_sub_epi64(_mm512_xor_si512(digits, sign), sign);

            // Variable-time table selection with skip
            ge_cached_8x sel;
            ge_cached_0_8x(&sel);

            const ge_cached_8x *T = tables.data() + g * 8;
            for (int j = 1; j <= 8; j++)
            {
                __mmask8 eq = _mm512_cmpeq_epi64_mask(abs_digits, _mm512_set1_epi64(j));
                if (eq == 0)
                    continue; // VT skip
                ge_cached_cmov_8x(&sel, &T[j - 1], eq);
            }

            ge_cached_cneg_8x(&sel, neg_mask);
            ge_p1p1_8x t2;
            ge_add_8x(&t2, &acc, &sel);
            ge_p1p1_to_p3_8x(&acc, &t2);
        }

        // Convert back to p2 for next round of doublings
        fe51x8_copy(&r.X, &acc.X);
        fe51x8_copy(&r.Y, &acc.Y);
        fe51x8_copy(&r.Z, &acc.Z);
    }

    // Extract 8 partial sums and combine
    ge_p3 partial[8];
    for (int k = 0; k < 8; k++)
        ge_p3_8x_extract_p3(&partial[k], &acc, k);

    // Start with partial[0], add remaining partials
    *result = partial[0];
    for (size_t k = 1; k < (n < 8 ? n : 8); k++)
    {
        ge_cached cached;
        ge_p3_to_cached(&cached, &partial[k]);
        ge_p1p1 t;
        ge_add(&t, result, &cached);
        ge_p1p1_to_p3(result, &t);
    }
}

// ============================================================================
// IFMA inline ge operations for Pippenger (single-point, IFMA field ops)
// ============================================================================

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
    fe_normalize_weak(r->Z);
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

static FE_IFMA_FORCE_INLINE void ge_p3_to_cached_ifma(ge_cached *r, const ge_p3 *p)
{
    static const fe d2 = {
        0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};
    fe_add(r->YplusX, p->Y, p->X);
    fe_sub(r->YminusX, p->Y, p->X);
    fe_copy(r->Z, p->Z);
    fe_ifma_chain_mul_nn(r->T2d, p->T, d2);
}

// ============================================================================
// IFMA Pippenger — used for n > 32
// ============================================================================

static int pippenger_window_size(size_t n)
{
    if (n < 96)
        return 5;
    if (n < 288)
        return 6;
    if (n < 864)
        return 7;
    if (n < 2592)
        return 8;
    if (n < 7776)
        return 9;
    if (n < 23328)
        return 10;
    return 11;
}

static void msm_pippenger_ifma(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    const int w = pippenger_window_size(n);
    const int num_buckets = (1 << (w - 1));
    const int num_windows = (256 + w - 1) / w;

    std::vector<signed char> all_digits(n * num_windows);
    for (size_t i = 0; i < n; i++)
        encode_signed_wbit(all_digits.data() + i * num_windows, scalars + i * 32, w);

    ge_p3 total;
    ge_p3_0(&total);

    for (int win = num_windows - 1; win >= 0; win--)
    {
        // Horner step: multiply accumulated result by 2^w
        if (win != num_windows - 1)
        {
            ge_p2 r2;
            ge_p3_to_p2(&r2, &total);
            ge_p1p1 t;
            for (int d = 0; d < w; d++)
            {
                ge_p2_dbl_ifma(&t, &r2);
                if (d < w - 1)
                    ge_p1p1_to_p2_ifma(&r2, &t);
                else
                    ge_p1p1_to_p3_ifma(&total, &t);
            }
        }

        // Initialize buckets
        std::vector<ge_p3> bucket_points(num_buckets);
        std::vector<bool> bucket_is_identity(num_buckets, true);
        for (int j = 0; j < num_buckets; j++)
            ge_p3_0(&bucket_points[j]);

        // Distribute points into buckets
        for (size_t i = 0; i < n; i++)
        {
            signed char digit = all_digits[i * num_windows + win];
            if (digit == 0)
                continue;

            int bucket_idx;
            ge_p3 effective_point;

            if (digit > 0)
            {
                bucket_idx = digit - 1;
                effective_point = points[i];
            }
            else
            {
                bucket_idx = (-digit) - 1;
                fe_neg(effective_point.X, points[i].X);
                fe_copy(effective_point.Y, points[i].Y);
                fe_copy(effective_point.Z, points[i].Z);
                fe_neg(effective_point.T, points[i].T);
            }

            if (bucket_is_identity[bucket_idx])
            {
                bucket_points[bucket_idx] = effective_point;
                bucket_is_identity[bucket_idx] = false;
            }
            else
            {
                ge_cached cached;
                ge_p3_to_cached_ifma(&cached, &effective_point);
                ge_p1p1 t;
                ge_add_ifma(&t, &bucket_points[bucket_idx], &cached);
                ge_p1p1_to_p3_ifma(&bucket_points[bucket_idx], &t);
            }
        }

        // Combine buckets using running sum
        ge_p3 running;
        ge_p3_0(&running);
        bool running_is_identity = true;

        ge_p3 partial;
        ge_p3_0(&partial);
        bool partial_is_identity = true;

        for (int j = num_buckets - 1; j >= 0; j--)
        {
            if (!bucket_is_identity[j])
            {
                if (running_is_identity)
                {
                    running = bucket_points[j];
                    running_is_identity = false;
                }
                else
                {
                    ge_cached cached;
                    ge_p3_to_cached_ifma(&cached, &bucket_points[j]);
                    ge_p1p1 t;
                    ge_add_ifma(&t, &running, &cached);
                    ge_p1p1_to_p3_ifma(&running, &t);
                }
            }

            if (!running_is_identity)
            {
                if (partial_is_identity)
                {
                    partial = running;
                    partial_is_identity = false;
                }
                else
                {
                    ge_cached cached;
                    ge_p3_to_cached_ifma(&cached, &running);
                    ge_p1p1 t;
                    ge_add_ifma(&t, &partial, &cached);
                    ge_p1p1_to_p3_ifma(&partial, &t);
                }
            }
        }

        // Add this window's result to total
        if (!partial_is_identity)
        {
            ge_cached cached;
            ge_p3_to_cached_ifma(&cached, &partial);
            ge_p1p1 t;
            ge_add_ifma(&t, &total, &cached);
            ge_p1p1_to_p3_ifma(&total, &t);
        }
    }

    *result = total;
}

// ============================================================================
// Public API (IFMA)
// ============================================================================

static const size_t STRAUS_PIPPENGER_CROSSOVER = 32;

void ge_msm_vartime_ifma(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    if (n == 0)
    {
        ge_p3_0(result);
        return;
    }

    if (n <= STRAUS_PIPPENGER_CROSSOVER)
        msm_straus_ifma(result, scalars, points, n);
    else
        msm_pippenger_ifma(result, scalars, points, n);
}

void ge_msm_base_vartime_ifma(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar)
{
    ge_p1p1 base_tmp;
    ge_p3 base_result;
    ge_scalarmult_base_ct(&base_tmp, base_scalar);
    ge_p1p1_to_p3(&base_result, &base_tmp);

    if (n == 0)
    {
        *result = base_result;
        return;
    }

    ge_p3 msm_result;
    ge_msm_vartime_ifma(&msm_result, scalars, points, n);

    ge_cached cached;
    ge_p3_to_cached(&cached, &msm_result);
    ge_p1p1 t;
    ge_add(&t, &base_result, &cached);
    ge_p1p1_to_p3(result, &t);
}
