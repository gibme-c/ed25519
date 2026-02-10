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
 * @file x64/ge_multiscalar_mul_vartime.cpp
 * @brief x64 multi-scalar multiplication: Straus (n<=32) and Pippenger (n>32).
 */

#include "ge_multiscalar_mul_vartime.h"

#include "fe_copy.h"
#include "fe_neg.h"
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

#include <cstring>
#include <vector>

// ============================================================================
// Signed digit encoding
// ============================================================================

/**
 * Encode a 256-bit scalar into 64 signed digits in [-8, 8] using radix-16.
 * Same encoding as ge_scalarmult_ct (4-bit fixed window).
 */
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

/**
 * Encode a 256-bit scalar into ceil(256/w) signed digits in [-(2^(w-1)), 2^(w-1)].
 * Uses radix-2^w with carry propagation for sign handling.
 * Returns the number of digits written.
 */
static int encode_signed_wbit(signed char *digits, const unsigned char *scalar, int w)
{
    const int half = 1 << (w - 1); // 2^(w-1)
    const int mask = (1 << w) - 1; // 2^w - 1
    const int num_digits = (256 + w - 1) / w;

    // Interpret scalar as a stream of bits
    int carry = 0;
    for (int i = 0; i < num_digits; i++)
    {
        // Extract w bits starting at bit position i*w
        int bit_pos = i * w;
        int byte_pos = bit_pos / 8;
        int bit_off = bit_pos % 8;

        // Gather enough bits (up to 16 bits to handle cross-byte boundaries)
        int raw = 0;
        if (byte_pos < 32)
            raw = scalar[byte_pos] >> bit_off;
        if (byte_pos + 1 < 32 && bit_off + w > 8)
            raw |= static_cast<int>(scalar[byte_pos + 1]) << (8 - bit_off);
        if (byte_pos + 2 < 32 && bit_off + w > 16)
            raw |= static_cast<int>(scalar[byte_pos + 2]) << (16 - bit_off);

        // Mask to w bits first, then add carry to handle overflow correctly
        int val = (raw & mask) + carry;
        carry = val >> w; // carry-out from addition
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
// Straus (interleaved) method — used for n <= 32
// ============================================================================

/**
 * Straus interleaved multi-scalar multiplication.
 * For each point, precompute table {1P, 2P, ..., 8P} as ge_cached.
 * Main loop: 64 rounds of 4 doublings + per-point add/sub for nonzero digits.
 */
static void msm_straus(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    // Encode all scalars into signed 4-bit digits
    std::vector<signed char> all_digits(n * 64);
    for (size_t i = 0; i < n; i++)
    {
        encode_signed_w4(all_digits.data() + i * 64, scalars + i * 32);
    }

    // Precompute tables: table[i][j] = (j+1) * points[i] as ge_cached
    std::vector<ge_cached> tables(n * 8);
    for (size_t i = 0; i < n; i++)
    {
        ge_cached *Ti = tables.data() + i * 8;
        ge_p3_to_cached(&Ti[0], &points[i]);
        for (int j = 0; j < 7; j++)
        {
            ge_p1p1 t;
            ge_p3 u;
            ge_add(&t, &points[i], &Ti[j]);
            ge_p1p1_to_p3(&u, &t);
            ge_p3_to_cached(&Ti[j + 1], &u);
        }
    }

    // Main loop: process digits from most significant to least
    ge_p2 r;
    ge_p2_0(&r);
    ge_p3 u;

    for (int d = 63; d >= 0; d--)
    {
        // 4 doublings
        ge_p1p1 t;
        ge_p2_dbl(&t, &r);
        ge_p1p1_to_p2(&r, &t);
        ge_p2_dbl(&t, &r);
        ge_p1p1_to_p2(&r, &t);
        ge_p2_dbl(&t, &r);
        ge_p1p1_to_p2(&r, &t);
        ge_p2_dbl(&t, &r);

        // Add contributions from each scalar
        ge_p1p1_to_p3(&u, &t);

        for (size_t i = 0; i < n; i++)
        {
            signed char digit = all_digits[i * 64 + d];
            if (digit > 0)
            {
                ge_add(&t, &u, &tables[i * 8 + digit - 1]);
                ge_p1p1_to_p3(&u, &t);
            }
            else if (digit < 0)
            {
                ge_sub(&t, &u, &tables[i * 8 + (-digit) - 1]);
                ge_p1p1_to_p3(&u, &t);
            }
        }

        // Convert back to p2 for next round of doublings
        ge_p3_to_p2(&r, &u);
    }

    // u holds the final result as ge_p3 from the last iteration
    *result = u;
}

// ============================================================================
// Pippenger (bucket method) — used for n > 32
// ============================================================================

/**
 * Select optimal window size for Pippenger based on n.
 * Minimizes total cost: n*ceil(256/w) additions + 2^(w-1)*ceil(256/w) running-sum + 256 doublings.
 */
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

/**
 * Pippenger bucket method multi-scalar multiplication.
 * For each window of w bits:
 *   1. Sort points into 2^(w-1) buckets by their signed digit
 *   2. Compute running sum and partial sum to combine all buckets
 * Then combine windows using Horner's method with w doublings per step.
 */
static void msm_pippenger(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    const int w = pippenger_window_size(n);
    const int num_buckets = (1 << (w - 1)); // 2^(w-1) buckets for signed digits
    const int num_windows = (256 + w - 1) / w;

    // Encode all scalars
    std::vector<signed char> all_digits(n * num_windows);
    for (size_t i = 0; i < n; i++)
    {
        encode_signed_wbit(all_digits.data() + i * num_windows, scalars + i * 32, w);
    }

    // Process windows from most significant to least (Horner's method)
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
                ge_p2_dbl(&t, &r2);
                if (d < w - 1)
                    ge_p1p1_to_p2(&r2, &t);
                else
                    ge_p1p1_to_p3(&total, &t);
            }
        }

        // Initialize buckets
        // bucket_points[j] accumulates all points with digit == (j+1)
        // bucket_is_identity[j] tracks whether the bucket is still identity
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
                // Negate the point: (X, Y, Z, T) → (-X, Y, Z, -T)
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
                ge_p3_to_cached(&cached, &effective_point);
                ge_p1p1 t;
                ge_add(&t, &bucket_points[bucket_idx], &cached);
                ge_p1p1_to_p3(&bucket_points[bucket_idx], &t);
            }
        }

        // Combine buckets: compute sum = 1*B_1 + 2*B_2 + ... + m*B_m
        // Using running sum: iterate from B_m down to B_1
        //   running = B_m, partial = B_m
        //   running += B_{m-1}, partial += running (= B_m + B_{m-1})
        //   running += B_{m-2}, partial += running (= 2*B_m + 2*B_{m-1} + B_{m-2})
        //   ... final partial = m*B_m + (m-1)*B_{m-1} + ... + 1*B_1
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
                    ge_p3_to_cached(&cached, &bucket_points[j]);
                    ge_p1p1 t;
                    ge_add(&t, &running, &cached);
                    ge_p1p1_to_p3(&running, &t);
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
                    ge_p3_to_cached(&cached, &running);
                    ge_p1p1 t;
                    ge_add(&t, &partial, &cached);
                    ge_p1p1_to_p3(&partial, &t);
                }
            }
        }

        // Add this window's result to total
        if (!partial_is_identity)
        {
            ge_cached cached;
            ge_p3_to_cached(&cached, &partial);
            ge_p1p1 t;
            ge_add(&t, &total, &cached);
            ge_p1p1_to_p3(&total, &t);
        }
    }

    *result = total;
}

// ============================================================================
// Public API (x64)
// ============================================================================

static const size_t STRAUS_PIPPENGER_CROSSOVER = 32;

void ge_msm_vartime_x64(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    if (n == 0)
    {
        ge_p3_0(result);
        return;
    }

    if (n <= STRAUS_PIPPENGER_CROSSOVER)
    {
        msm_straus(result, scalars, points, n);
    }
    else
    {
        msm_pippenger(result, scalars, points, n);
    }
}

void ge_msm_base_vartime_x64(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar)
{
    // Compute base_scalar * B using the precomputed base table
    ge_p1p1 base_tmp;
    ge_p3 base_result;
    ge_scalarmult_base_ct(&base_tmp, base_scalar);
    ge_p1p1_to_p3(&base_result, &base_tmp);

    if (n == 0)
    {
        *result = base_result;
        return;
    }

    // Compute MSM
    ge_p3 msm_result;
    ge_msm_vartime_x64(&msm_result, scalars, points, n);

    // Add base_result + msm_result
    ge_cached cached;
    ge_p3_to_cached(&cached, &msm_result);
    ge_p1p1 t;
    ge_add(&t, &base_result, &cached);
    ge_p1p1_to_p3(result, &t);
}
