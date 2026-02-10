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
 * @file ifma/ge_dsm_base_negate_vt_batch.cpp
 * @brief IFMA 8-way batch double scalar multiplication for verification.
 *
 * Uses the same 4-bit windowed approach as the CT batch for SIMD friendliness.
 * Both A-table and B-table selections use per-lane k-mask compare with
 * ge_cached_8x. Zero-digit lanes are handled via blending.
 */

#include "ge.h"
#include "ge_add.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_to_cached.h"
#include "ge_scalarmult_base_ct.h"
#include "x64/ifma/ge_batch_ifma.h"

// d2 = 2*d constant in fe51
static const fe ge_dsm_batch_ifma_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

/**
 * @brief Build the base point table Bi[8] = {1B, 2B, ..., 8B} as ge_cached.
 */
static void build_base_table_ifma(ge_cached Bi_scalar[8])
{
    unsigned char one[32] = {};
    one[0] = 1;
    ge_p1p1 tmp;
    ge_scalarmult_base_ct(&tmp, one);
    ge_p3 B;
    ge_p1p1_to_p3(&B, &tmp);

    ge_p3_to_cached(&Bi_scalar[0], &B);

    ge_p3 acc;
    for (int j = 0; j < 7; j++)
    {
        ge_add(&tmp, &B, &Bi_scalar[j]);
        ge_p1p1_to_p3(&acc, &tmp);
        ge_p3_to_cached(&Bi_scalar[j + 1], &acc);
    }
}

void ge_dsm_base_negate_vt_batch_ifma(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count)
{
    if (count == 0)
        return;

    // Broadcast d2 constant
    fe51x8 d2_8x;
    fe51x8_broadcast_d2(&d2_8x, ge_dsm_batch_ifma_d2_fe51);

    // Build B-table: consecutive multiples {1B, 2B, ..., 8B}
    ge_cached Bi_scalar[8];
    build_base_table_ifma(Bi_scalar);

    // Broadcast into all 8 lanes
    ge_cached_8x Bi_8x[8];
    for (int j = 0; j < 8; j++)
        ge_cached_broadcast_8x(&Bi_8x[j], &Bi_scalar[j]);

    for (size_t batch = 0; batch < count; batch += 8)
    {
        size_t n = (count - batch < 8) ? (count - batch) : 8;

        // Encode a-scalars and b-scalars into signed 4-bit digits
        signed char ae[8][64], be[8][64];
        for (size_t k = 0; k < 8; k++)
        {
            if (k < n)
            {
                auto encode = [](signed char *e, const unsigned char *s)
                {
                    int carry = 0, carry2;
                    for (int i = 0; i < 31; i++)
                    {
                        carry += s[i];
                        carry2 = (carry + 8) >> 4;
                        e[2 * i] = (signed char)(carry - (carry2 << 4));
                        carry = (carry2 + 8) >> 4;
                        e[2 * i + 1] = (signed char)(carry2 - (carry << 4));
                    }
                    carry += s[31];
                    carry2 = (carry + 8) >> 4;
                    e[62] = (signed char)(carry - (carry2 << 4));
                    e[63] = (signed char)carry2;
                };
                encode(ae[k], a_scalars + (batch + k) * 32);
                encode(be[k], b_scalars + (batch + k) * 32);
            }
            else
            {
                for (int i = 0; i < 64; i++)
                {
                    ae[k][i] = 0;
                    be[k][i] = 0;
                }
            }
        }

        // Pack A points
        ge_p3_8x A8x;
        ge_p3_0_8x(&A8x);
        for (size_t k = 0; k < n; k++)
            ge_p3_8x_insert(&A8x, &A_points[batch + k], (int)k);

        // Build A-table: Ai[j] = (j+1)*A
        alignas(64) ge_cached_8x Ai[8];
        ge_p3_8x u;
        ge_p1p1_8x t;

        ge_p3_to_cached_8x(&Ai[0], &A8x, &d2_8x);
        for (int i = 0; i < 7; i++)
        {
            ge_add_8x(&t, &A8x, &Ai[i]);
            ge_p1p1_to_p3_8x(&u, &t);
            ge_p3_to_cached_8x(&Ai[i + 1], &u, &d2_8x);
        }

        // Initialize r to identity
        ge_p2_8x r;
        ge_p2_0_8x(&r);

        for (int d = 63; d >= 0; d--)
        {
            // 4 doublings
            ge_p2_dbl_8x(&t, &r);
            ge_p1p1_to_p2_8x(&r, &t);
            ge_p2_dbl_8x(&t, &r);
            ge_p1p1_to_p2_8x(&r, &t);
            ge_p2_dbl_8x(&t, &r);
            ge_p1p1_to_p2_8x(&r, &t);
            ge_p2_dbl_8x(&t, &r);
            ge_p1p1_to_p3_8x(&u, &t);

            // A-table selection
            {
                __m512i a_digits = _mm512_set_epi64(
                    (long long)ae[7][d],
                    (long long)ae[6][d],
                    (long long)ae[5][d],
                    (long long)ae[4][d],
                    (long long)ae[3][d],
                    (long long)ae[2][d],
                    (long long)ae[1][d],
                    (long long)ae[0][d]);
                const __m512i zero_vec = _mm512_setzero_si512();
                __mmask8 a_neg_mask = _mm512_cmpgt_epi64_mask(zero_vec, a_digits);
                __m512i a_sign = _mm512_srai_epi64(a_digits, 63);
                __m512i a_abs = _mm512_sub_epi64(_mm512_xor_si512(a_digits, a_sign), a_sign);
                __mmask8 a_nonzero = ~_mm512_cmpeq_epi64_mask(a_abs, _mm512_setzero_si512());

                ge_cached_8x cur_a;
                ge_cached_0_8x(&cur_a);
                for (int j = 1; j <= 8; j++)
                {
                    __mmask8 eq = _mm512_cmpeq_epi64_mask(a_abs, _mm512_set1_epi64(j));
                    ge_cached_cmov_8x(&cur_a, &Ai[j - 1], eq);
                }
                ge_cached_cneg_8x(&cur_a, a_neg_mask);

                ge_p1p1_8x t_a;
                ge_add_8x(&t_a, &u, &cur_a);
                ge_p3_8x new_u;
                ge_p1p1_to_p3_8x(&new_u, &t_a);
                // Blend: keep u where a_digit == 0
                ge_p3_blend_8x(&u, &u, &new_u, a_nonzero);
            }

            // B-table selection
            {
                __m512i b_digits = _mm512_set_epi64(
                    (long long)be[7][d],
                    (long long)be[6][d],
                    (long long)be[5][d],
                    (long long)be[4][d],
                    (long long)be[3][d],
                    (long long)be[2][d],
                    (long long)be[1][d],
                    (long long)be[0][d]);
                const __m512i zero_b = _mm512_setzero_si512();
                __mmask8 b_neg_mask = _mm512_cmpgt_epi64_mask(zero_b, b_digits);
                __m512i b_sign = _mm512_srai_epi64(b_digits, 63);
                __m512i b_abs = _mm512_sub_epi64(_mm512_xor_si512(b_digits, b_sign), b_sign);
                __mmask8 b_nonzero = ~_mm512_cmpeq_epi64_mask(b_abs, _mm512_setzero_si512());

                ge_cached_8x cur_b;
                ge_cached_0_8x(&cur_b);
                for (int j = 1; j <= 8; j++)
                {
                    __mmask8 eq = _mm512_cmpeq_epi64_mask(b_abs, _mm512_set1_epi64(j));
                    ge_cached_cmov_8x(&cur_b, &Bi_8x[j - 1], eq);
                }
                ge_cached_cneg_8x(&cur_b, b_neg_mask);

                ge_p1p1_8x t_b;
                ge_add_8x(&t_b, &u, &cur_b);
                ge_p2_8x new_r;
                ge_p1p1_to_p2_8x(&new_r, &t_b);
                // For zero b-digit lanes, convert u to p2 directly
                ge_p2_8x u_as_p2;
                fe51x8_copy(&u_as_p2.X, &u.X);
                fe51x8_copy(&u_as_p2.Y, &u.Y);
                fe51x8_copy(&u_as_p2.Z, &u.Z);
                ge_p2_blend_8x(&r, &u_as_p2, &new_r, b_nonzero);
            }
        }

        for (size_t k = 0; k < n; k++)
            ge_p2_8x_extract(&results[batch + k], &r, (int)k);
    }
}
