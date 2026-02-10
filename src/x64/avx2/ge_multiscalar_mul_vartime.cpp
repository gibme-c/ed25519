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
 * @file avx2/ge_multiscalar_mul_vartime.cpp
 * @brief AVX2 4-way multi-scalar multiplication: 4-way Straus + x64 Pippenger fallback.
 *
 * Straus (n<=32): splits n points across groups of 4, processing each group
 * in parallel using fe10x4 field arithmetic. Each group has its own 4-way
 * precomputed table, and the accumulator holds 4 partial sums that are
 * combined at the end.
 *
 * Pippenger (n>32): falls back to the x64 baseline implementation (AVX2
 * doesn't help for single-point variable-time code without table cmov).
 */

#include "ge_multiscalar_mul_vartime.h"

#include "ge_add.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_0.h"
#include "ge_p3_to_cached.h"
#include "ge_scalarmult_base_ct.h"
#include "x64/avx2/ge_batch_avx2.h"

#include <cstring>
#include <vector>

// ============================================================================
// Signed digit encoding
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

// ============================================================================
// AVX2 4-way Straus — used for n <= 32
// ============================================================================

// d2 = 2*d constant in fe51
static const fe msm_avx2_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

static void msm_straus_avx2(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    // Broadcast d2 constant
    fe10x4 d2_4x;
    fe10x4_broadcast_d2(&d2_4x, msm_avx2_d2_fe51);

    // Encode all scalars into signed 4-bit digits
    std::vector<signed char> all_digits(n * 64);
    for (size_t i = 0; i < n; i++)
        encode_signed_w4(all_digits.data() + i * 64, scalars + i * 32);

    const size_t num_groups = (n + 3) / 4;

    // Precompute tables: one 4-way table per group of 4 points
    std::vector<ge_cached_4x> tables(num_groups * 8);

    for (size_t g = 0; g < num_groups; g++)
    {
        size_t base = g * 4;
        size_t group_n = (n - base < 4) ? (n - base) : 4;

        ge_p3_4x P4x;
        ge_p3_0_4x(&P4x);
        for (size_t k = 0; k < group_n; k++)
            ge_p3_4x_insert(&P4x, &points[base + k], (int)k);

        ge_cached_4x *T = tables.data() + g * 8;
        ge_p3_to_cached_4x(&T[0], &P4x, &d2_4x);
        for (int j = 0; j < 7; j++)
        {
            ge_p1p1_4x t;
            ge_p3_4x u;
            ge_add_4x(&t, &P4x, &T[j]);
            ge_p1p1_to_p3_4x(&u, &t);
            ge_p3_to_cached_4x(&T[j + 1], &u, &d2_4x);
        }
    }

    // Main loop: accumulator holds 4 partial sums
    ge_p3_4x acc;
    ge_p3_0_4x(&acc);
    ge_p2_4x r;
    ge_p2_0_4x(&r);

    for (int d = 63; d >= 0; d--)
    {
        // 4 doublings
        ge_p1p1_4x t;
        ge_p2_dbl_4x(&t, &r);
        ge_p1p1_to_p2_4x(&r, &t);
        ge_p2_dbl_4x(&t, &r);
        ge_p1p1_to_p2_4x(&r, &t);
        ge_p2_dbl_4x(&t, &r);
        ge_p1p1_to_p2_4x(&r, &t);
        ge_p2_dbl_4x(&t, &r);
        ge_p1p1_to_p3_4x(&acc, &t);

        // Process each group of 4 points
        for (size_t g = 0; g < num_groups; g++)
        {
            size_t base = g * 4;
            size_t group_n = (n - base < 4) ? (n - base) : 4;

            // Load per-lane digits
            int64_t lane_digits[4] = {0, 0, 0, 0};
            for (size_t k = 0; k < group_n; k++)
                lane_digits[k] = (int64_t)all_digits[(base + k) * 64 + d];

            __m256i digits = _mm256_loadu_si256((const __m256i *)lane_digits);

            // Check if all digits are zero
            __m256i zero_vec = _mm256_setzero_si256();
            __m256i neq = _mm256_xor_si256(digits, zero_vec);
            if (_mm256_testz_si256(neq, neq))
                continue;

            // Compute abs_digits and neg_mask
            __m256i neg_mask = _mm256_cmpgt_epi64(zero_vec, digits);
            __m256i abs_digits = _mm256_sub_epi64(_mm256_xor_si256(digits, neg_mask), neg_mask);

            // Variable-time table selection with skip
            ge_cached_4x sel;
            ge_cached_0_4x(&sel);

            const ge_cached_4x *T = tables.data() + g * 8;
            for (int j = 1; j <= 8; j++)
            {
                __m256i eq = _mm256_cmpeq_epi64(abs_digits, _mm256_set1_epi64x(j));
                if (_mm256_testz_si256(eq, eq))
                    continue; // VT skip
                ge_cached_cmov_4x(&sel, &T[j - 1], eq);
            }

            ge_cached_cneg_4x(&sel, neg_mask);
            ge_p1p1_4x t2;
            ge_add_4x(&t2, &acc, &sel);
            ge_p1p1_to_p3_4x(&acc, &t2);
        }

        // Convert back to p2 for next round
        fe10x4_copy(&r.X, &acc.X);
        fe10x4_copy(&r.Y, &acc.Y);
        fe10x4_copy(&r.Z, &acc.Z);
    }

    // Extract 4 partial sums and combine
    ge_p3 partial[4];
    for (int k = 0; k < 4; k++)
        ge_p3_4x_extract_p3(&partial[k], &acc, k);

    *result = partial[0];
    for (size_t k = 1; k < (n < 4 ? n : 4); k++)
    {
        ge_cached cached;
        ge_p3_to_cached(&cached, &partial[k]);
        ge_p1p1 t;
        ge_add(&t, result, &cached);
        ge_p1p1_to_p3(result, &t);
    }
}

// ============================================================================
// Public API (AVX2)
// ============================================================================

// Forward declaration: x64 baseline Pippenger fallback
void ge_msm_vartime_x64(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n);

static const size_t STRAUS_PIPPENGER_CROSSOVER = 32;

void ge_msm_vartime_avx2(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    if (n == 0)
    {
        ge_p3_0(result);
        return;
    }

    if (n <= STRAUS_PIPPENGER_CROSSOVER)
        msm_straus_avx2(result, scalars, points, n);
    else
        ge_msm_vartime_x64(result, scalars, points, n); // x64 baseline Pippenger
}

void ge_msm_base_vartime_avx2(
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
    ge_msm_vartime_avx2(&msm_result, scalars, points, n);

    ge_cached cached;
    ge_p3_to_cached(&cached, &msm_result);
    ge_p1p1 t;
    ge_add(&t, &base_result, &cached);
    ge_p1p1_to_p3(result, &t);
}
