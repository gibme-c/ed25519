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
 * @file avx2/ge_dsm_base_negate_vt_batch.cpp
 * @brief AVX2 4-way batch double scalar multiplication for verification.
 *
 * Uses the same 4-bit windowed approach as the CT batch for SIMD friendliness.
 * Both A-table and B-table selections use per-lane masking with ge_cached_4x.
 * Zero-digit lanes are handled via blending (keep previous value).
 */

#include "ge.h"
#include "ge_add.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_to_cached.h"
#include "ge_scalarmult_base_ct.h"
#include "negative.h"
#include "x64/avx2/ge_batch_avx2.h"

// d2 = 2*d constant in fe51
static const fe ge_dsm_batch_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

/**
 * @brief Broadcast a scalar ge_cached into all 4 lanes of ge_cached_4x.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_cached_broadcast_4x(ge_cached_4x *out, const ge_cached *in)
{
    fe10 ypx, ymx, z10, t2d;
    fe51_to_fe10(ypx, in->YplusX);
    fe51_to_fe10(ymx, in->YminusX);
    fe51_to_fe10(z10, in->Z);
    fe51_to_fe10(t2d, in->T2d);
    for (int i = 0; i < 10; i++)
    {
        out->YplusX.v[i] = _mm256_set1_epi64x(ypx[i]);
        out->YminusX.v[i] = _mm256_set1_epi64x(ymx[i]);
        out->Z.v[i] = _mm256_set1_epi64x(z10[i]);
        out->T2d.v[i] = _mm256_set1_epi64x(t2d[i]);
    }
}

/**
 * @brief Build the base point table Bi[8] = {1B, 2B, ..., 8B} as ge_cached.
 *
 * Computes B = 1*G via ge_scalarmult_base_ct, then builds consecutive multiples
 * using scalar ge operations (done once per function call, not per batch).
 */
static void build_base_table(ge_cached Bi_scalar[8])
{
    // Compute base point B = 1 * G
    unsigned char one[32] = {};
    one[0] = 1;
    ge_p1p1 tmp;
    ge_scalarmult_base_ct(&tmp, one);
    ge_p3 B;
    ge_p1p1_to_p3(&B, &tmp);

    // Bi_scalar[0] = 1B (as cached)
    ge_p3_to_cached(&Bi_scalar[0], &B);

    // Bi_scalar[j] = (j+1)*B
    ge_p3 acc;
    for (int j = 0; j < 7; j++)
    {
        ge_add(&tmp, &B, &Bi_scalar[j]);
        ge_p1p1_to_p3(&acc, &tmp);
        ge_p3_to_cached(&Bi_scalar[j + 1], &acc);
    }
}

void ge_dsm_base_negate_vt_batch_avx2(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count)
{
    if (count == 0)
        return;

    // Broadcast d2 constant
    fe10x4 d2_4x;
    fe10x4_broadcast_d2(&d2_4x, ge_dsm_batch_d2_fe51);

    // Build B-table: consecutive multiples {1B, 2B, ..., 8B}
    ge_cached Bi_scalar[8];
    build_base_table(Bi_scalar);

    // Convert to ge_cached_4x (broadcast same B-table into all 4 lanes)
    ge_cached_4x Bi_4x[8];
    for (int j = 0; j < 8; j++)
        ge_cached_broadcast_4x(&Bi_4x[j], &Bi_scalar[j]);

    for (size_t batch = 0; batch < count; batch += 4)
    {
        size_t n = (count - batch < 4) ? (count - batch) : 4;

        // Encode a-scalars and b-scalars into signed 4-bit digits
        signed char ae[4][64], be[4][64];
        for (size_t k = 0; k < 4; k++)
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
        ge_p3_4x A4x;
        ge_p3_0_4x(&A4x);
        for (size_t k = 0; k < n; k++)
            ge_p3_4x_insert(&A4x, &A_points[batch + k], (int)k);

        // Build A-table: Ai[j] = (j+1)*A
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

            // A-table selection (ge_cached)
            {
                __m256i a_digits =
                    _mm256_set_epi64x((int64_t)ae[3][d], (int64_t)ae[2][d], (int64_t)ae[1][d], (int64_t)ae[0][d]);
                __m256i a_neg_mask = _mm256_cmpgt_epi64(_mm256_setzero_si256(), a_digits);
                __m256i a_abs = _mm256_sub_epi64(_mm256_xor_si256(a_digits, a_neg_mask), a_neg_mask);
                __m256i a_nonzero =
                    _mm256_xor_si256(_mm256_cmpeq_epi64(a_abs, _mm256_setzero_si256()), _mm256_set1_epi64x(-1LL));

                ge_cached_4x cur_a;
                ge_cached_0_4x(&cur_a);
                for (int j = 1; j <= 8; j++)
                {
                    __m256i eq = _mm256_cmpeq_epi64(a_abs, _mm256_set1_epi64x(j));
                    ge_cached_cmov_4x(&cur_a, &Ai[j - 1], eq);
                }
                ge_cached_cneg_4x(&cur_a, a_neg_mask);

                ge_p1p1_4x t_a;
                ge_add_4x(&t_a, &u, &cur_a);
                ge_p3_4x new_u;
                ge_p1p1_to_p3_4x(&new_u, &t_a);
                // Blend: keep u where a_digit == 0
                ge_p3_blend_4x(&u, &u, &new_u, a_nonzero);
            }

            // B-table selection (ge_cached, same approach as A-table)
            {
                __m256i b_digits =
                    _mm256_set_epi64x((int64_t)be[3][d], (int64_t)be[2][d], (int64_t)be[1][d], (int64_t)be[0][d]);
                __m256i b_neg_mask = _mm256_cmpgt_epi64(_mm256_setzero_si256(), b_digits);
                __m256i b_abs = _mm256_sub_epi64(_mm256_xor_si256(b_digits, b_neg_mask), b_neg_mask);
                __m256i b_nonzero =
                    _mm256_xor_si256(_mm256_cmpeq_epi64(b_abs, _mm256_setzero_si256()), _mm256_set1_epi64x(-1LL));

                ge_cached_4x cur_b;
                ge_cached_0_4x(&cur_b);
                for (int j = 1; j <= 8; j++)
                {
                    __m256i eq = _mm256_cmpeq_epi64(b_abs, _mm256_set1_epi64x(j));
                    ge_cached_cmov_4x(&cur_b, &Bi_4x[j - 1], eq);
                }
                ge_cached_cneg_4x(&cur_b, b_neg_mask);

                ge_p1p1_4x t_b;
                ge_add_4x(&t_b, &u, &cur_b);
                ge_p2_4x new_r;
                ge_p1p1_to_p2_4x(&new_r, &t_b);
                // For zero b-digit lanes, convert u to p2 directly
                ge_p2_4x u_as_p2;
                fe10x4_copy(&u_as_p2.X, &u.X);
                fe10x4_copy(&u_as_p2.Y, &u.Y);
                fe10x4_copy(&u_as_p2.Z, &u.Z);
                ge_p2_blend_4x(&r, &u_as_p2, &new_r, b_nonzero);
            }
        }

        for (size_t k = 0; k < n; k++)
            ge_p2_4x_extract(&results[batch + k], &r, (int)k);
    }
}
