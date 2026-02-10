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
 * @file avx2/ge_dsm_negate_vt_batch_ss_p3.cpp
 * @brief AVX2 4-way batch double scalar multiplication with shared scalars, returning ge_p3.
 *
 * Same algorithm as ge_dsm_negate_vt_batch_ss_avx2 but produces ge_p3 output
 * (extended coordinates with T = X*Y/Z). On the final iteration, the B-table
 * step converts p1p1 → p3 instead of p1p1 → p2, costing one extra field
 * multiply per point but saving the caller ~260 field ops of ge_p2_to_p3.
 */

#include "ge.h"
#include "ge_add.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_to_cached.h"
#include "x64/avx2/ge_batch_avx2.h"

// d2 = 2*d constant in fe51
static const fe ge_dsm_batch_ss_p3_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

/**
 * @brief Extract a ge_p3 from one lane of a ge_p3_4x.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_p3_4x_extract(ge_p3 *out, const ge_p3_4x *in, int lane)
{
    fe10 X10, Y10, Z10, T10;
    fe10x4_extract_lane(X10, &in->X, lane);
    fe10x4_extract_lane(Y10, &in->Y, lane);
    fe10x4_extract_lane(Z10, &in->Z, lane);
    fe10x4_extract_lane(T10, &in->T, lane);
    fe10_to_fe51(out->X, X10);
    fe10_to_fe51(out->Y, Y10);
    fe10_to_fe51(out->Z, Z10);
    fe10_to_fe51(out->T, T10);
}

void ge_dsm_negate_vt_batch_ss_p3_avx2(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count)
{
    if (count == 0)
        return;

    // Broadcast d2 constant
    fe10x4 d2_4x;
    fe10x4_broadcast_d2(&d2_4x, ge_dsm_batch_ss_p3_d2_fe51);

    // Encode shared scalars into signed 4-bit digits (once, not per-lane)
    signed char ae[64], be[64];
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
        encode(ae, a);
        encode(be, b);
    }

    for (size_t batch = 0; batch < count; batch += 4)
    {
        size_t n = (count - batch < 4) ? (count - batch) : 4;

        // Pack A points into 4-way
        ge_p3_4x A4x;
        ge_p3_0_4x(&A4x);
        for (size_t k = 0; k < n; k++)
            ge_p3_4x_insert(&A4x, &A_points[batch + k], (int)k);

        // Build A-table: Ai[j] = (j+1)*A_points (per-lane, different points)
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

        // Pack B points into 4-way and build B-table
        ge_p3_4x B4x;
        ge_p3_0_4x(&B4x);
        for (size_t k = 0; k < n; k++)
            ge_p3_4x_insert(&B4x, &B_points[batch + k], (int)k);

        alignas(64) ge_cached_4x Bi[8];
        ge_p3_to_cached_4x(&Bi[0], &B4x, &d2_4x);
        for (int i = 0; i < 7; i++)
        {
            ge_add_4x(&t, &B4x, &Bi[i]);
            ge_p1p1_to_p3_4x(&u, &t);
            ge_p3_to_cached_4x(&Bi[i + 1], &u, &d2_4x);
        }

        // Initialize r to identity
        ge_p2_4x r;
        ge_p2_0_4x(&r);

        ge_p3_4x final_p3;
        ge_p3_0_4x(&final_p3);

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

            // A-table selection: shared scalar digit → direct table index
            {
                int a_digit = ae[d];
                if (a_digit != 0)
                {
                    int a_idx = a_digit < 0 ? -a_digit : a_digit;
                    ge_p1p1_4x t_a;
                    if (a_digit > 0)
                        ge_add_4x(&t_a, &u, &Ai[a_idx - 1]);
                    else
                        ge_sub_4x(&t_a, &u, &Ai[a_idx - 1]);
                    ge_p1p1_to_p3_4x(&u, &t_a);
                }
            }

            // B-table selection: shared scalar digit → direct table index
            {
                int b_digit = be[d];
                if (b_digit != 0)
                {
                    int b_idx = b_digit < 0 ? -b_digit : b_digit;
                    ge_p1p1_4x t_b;
                    if (b_digit > 0)
                        ge_add_4x(&t_b, &u, &Bi[b_idx - 1]);
                    else
                        ge_sub_4x(&t_b, &u, &Bi[b_idx - 1]);

                    if (d > 0)
                        ge_p1p1_to_p2_4x(&r, &t_b);
                    else
                        ge_p1p1_to_p3_4x(&final_p3, &t_b);
                }
                else
                {
                    if (d > 0)
                    {
                        fe10x4_copy(&r.X, &u.X);
                        fe10x4_copy(&r.Y, &u.Y);
                        fe10x4_copy(&r.Z, &u.Z);
                    }
                    else
                    {
                        final_p3 = u;
                    }
                }
            }
        }

        for (size_t k = 0; k < n; k++)
            ge_p3_4x_extract(&results[batch + k], &final_p3, (int)k);
    }
}
