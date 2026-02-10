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
 * @file avx2/ge_scalarmult_ct_batch.cpp
 * @brief AVX2 4-way batch constant-time variable-base scalar multiplication.
 *
 * Processes 4 independent scalarmult operations in parallel using fe10x4
 * field arithmetic. Same algorithm as single-op ge_scalarmult_avx2_ct but
 * with 4 points per iteration.
 */

#include "ed25519_secure_erase.h"
#include "equal.h"
#include "ge.h"
#include "ge_p1p1_to_p2.h"
#include "negative.h"
#include "x64/avx2/ge_batch_avx2.h"

// d2 = 2*d constant in fe51
static const fe ge_batch_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

void ge_scalarmult_ct_batch_avx2(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count)
{
    if (count == 0)
        return;

    // Broadcast d2 constant into all 4 lanes
    fe10x4 d2_4x;
    fe10x4_broadcast_d2(&d2_4x, ge_batch_d2_fe51);

    for (size_t batch = 0; batch < count; batch += 4)
    {
        size_t n = (count - batch < 4) ? (count - batch) : 4;

        // Encode scalars into signed 4-bit digits
        alignas(64) signed char e[4][64];
        for (size_t k = 0; k < 4; k++)
        {
            if (k < n)
            {
                const unsigned char *a = scalars + (batch + k) * 32;
                int carry = 0, carry2;
                for (int i = 0; i < 31; i++)
                {
                    carry += a[i];
                    carry2 = (carry + 8) >> 4;
                    e[k][2 * i] = (signed char)(carry - (carry2 << 4));
                    carry = (carry2 + 8) >> 4;
                    e[k][2 * i + 1] = (signed char)(carry2 - (carry << 4));
                }
                carry += a[31];
                carry2 = (carry + 8) >> 4;
                e[k][62] = (signed char)(carry - (carry2 << 4));
                e[k][63] = (signed char)carry2;
            }
            else
            {
                // Pad with zero scalar (identity * 0 = identity)
                for (int i = 0; i < 64; i++)
                    e[k][i] = 0;
            }
        }

        // Pack 4 points into ge_p3_4x (padding with identity for unused lanes)
        ge_p3_4x A4x;
        ge_p3_0_4x(&A4x);
        for (size_t k = 0; k < n; k++)
            ge_p3_4x_insert(&A4x, &points[batch + k], (int)k);

        // Build Ai[8] table: Ai[j] = (j+1) * A for j=0..7
        alignas(64) ge_cached_4x Ai[8];
        ge_p3_4x u;
        ge_p1p1_4x t;

        ge_p3_to_cached_4x(&Ai[0], &A4x, &d2_4x);
        for (int i = 0; i < 7; i++)
        {
            ge_add_4x(&t, &A4x, &Ai[i]);
            ge_p1p1_to_p3_4x(&u, &t);
            ge_p3_to_cached_4x(&Ai[i + 1], &u, &d2_4x);
        }

        // Initialize r to identity
        ge_p2_4x r;
        ge_p2_0_4x(&r);

        // Main loop: process 4-bit digits from MSB to LSB
        for (int d = 63; d >= 0; d--)
        {
            // 4 doublings
            ge_p2_dbl_4x(&t, &r);
            ge_p1p1_to_p2_4x(&r, &t);
            ge_p2_dbl_4x(&t, &r);
            ge_p1p1_to_p2_4x(&r, &t);
            ge_p2_dbl_4x(&t, &r);
            ge_p1p1_to_p2_4x(&r, &t);
            ge_p2_dbl_4x(&t, &r);
            ge_p1p1_to_p3_4x(&u, &t);

            // Per-lane table selection using vectorized cmov
            // Build per-lane absolute digit values
            __m256i digits = _mm256_set_epi64x((int64_t)e[3][d], (int64_t)e[2][d], (int64_t)e[1][d], (int64_t)e[0][d]);
            // Compute bnegative per lane: sign bit extraction
            __m256i neg_mask = _mm256_cmpgt_epi64(_mm256_setzero_si256(), digits);
            // babs = b - (((-bneg) & b) << 1) = b XOR neg_mask, then subtract neg_mask
            // For signed: abs = (b XOR mask) - mask where mask = arithmetic_shift(b, 63)
            __m256i abs_digits = _mm256_sub_epi64(_mm256_xor_si256(digits, neg_mask), neg_mask);

            ge_cached_4x cur;
            ge_cached_0_4x(&cur);

            for (int j = 1; j <= 8; j++)
            {
                __m256i eq = _mm256_cmpeq_epi64(abs_digits, _mm256_set1_epi64x(j));
                ge_cached_cmov_4x(&cur, &Ai[j - 1], eq);
            }

            ge_cached_cneg_4x(&cur, neg_mask);
            ge_add_4x(&t, &u, &cur);
            ge_p1p1_to_p2_4x(&r, &t);
        }

        // Extract results
        for (size_t k = 0; k < n; k++)
            ge_p2_4x_extract(&results[batch + k], &r, (int)k);

        // Secure erase temporaries
        ed25519_secure_erase(e, sizeof(e));
        ed25519_secure_erase(&Ai, sizeof(Ai));
        ed25519_secure_erase(&u, sizeof(u));
        ed25519_secure_erase(&r, sizeof(r));
        ed25519_secure_erase(&t, sizeof(t));
    }
}
