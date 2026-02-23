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
 * @file ifma/ge_dsm_negate_vt_batch_ss_p3.cpp
 * @brief IFMA 8-way batch double scalar multiplication with shared scalars, returning ge_p3.
 *
 * Same algorithm as ge_dsm_negate_vt_batch_ss_ifma but produces ge_p3 output
 * (extended coordinates with T = X*Y/Z). On the final iteration, the B-table
 * step converts p1p1 → p3 instead of p1p1 → p2, costing one extra field
 * multiply per point but saving the caller ~260 field ops of ge_p2_to_p3.
 */

#include "ed25519_secure_erase.h"
#include "ge.h"
#include "ge_add.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_to_cached.h"
#include "x64/ifma/ge_batch_ifma.h"

// d2 = 2*d constant in fe51
static const fe ge_dsm_batch_ss_p3_ifma_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

/**
 * @brief Extract a ge_p3 from one lane of a ge_p3_8x.
 */
static GE_BATCH_IFMA_FORCE_INLINE void ge_p3_8x_extract(ge_p3 *out, const ge_p3_8x *in, int lane)
{
    fe51x8_extract_lane(out->X, &in->X, lane);
    fe51x8_extract_lane(out->Y, &in->Y, lane);
    fe51x8_extract_lane(out->Z, &in->Z, lane);
    fe51x8_extract_lane(out->T, &in->T, lane);
}

void ge_dsm_negate_vt_batch_ss_p3_ifma(
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
    fe51x8 d2_8x;
    fe51x8_broadcast_d2(&d2_8x, ge_dsm_batch_ss_p3_ifma_d2_fe51);

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

    for (size_t batch = 0; batch < count; batch += 8)
    {
        size_t n = (count - batch < 8) ? (count - batch) : 8;

        // Pack A points into 8-way
        ge_p3_8x A8x;
        ge_p3_0_8x(&A8x);
        for (size_t k = 0; k < n; k++)
            ge_p3_8x_insert(&A8x, &A_points[batch + k], (int)k);

        // Build A-table: Ai[j] = (j+1)*A_points (per-lane, different points)
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

        // Pack B points into 8-way and build B-table
        ge_p3_8x B8x;
        ge_p3_0_8x(&B8x);
        for (size_t k = 0; k < n; k++)
            ge_p3_8x_insert(&B8x, &B_points[batch + k], (int)k);

        alignas(64) ge_cached_8x Bi[8];
        ge_p3_to_cached_8x(&Bi[0], &B8x, &d2_8x);
        for (int i = 0; i < 7; i++)
        {
            ge_add_8x(&t, &B8x, &Bi[i]);
            ge_p1p1_to_p3_8x(&u, &t);
            ge_p3_to_cached_8x(&Bi[i + 1], &u, &d2_8x);
        }

        // Initialize r to identity
        ge_p2_8x r;
        ge_p2_0_8x(&r);

        ge_p3_8x final_p3;
        ge_p3_0_8x(&final_p3);

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

            // A-table selection: shared scalar digit → direct table index
            {
                int a_digit = ae[d];
                if (a_digit != 0)
                {
                    int a_idx = a_digit < 0 ? -a_digit : a_digit;
                    ge_p1p1_8x t_a;
                    if (a_digit > 0)
                        ge_add_8x(&t_a, &u, &Ai[a_idx - 1]);
                    else
                        ge_sub_8x(&t_a, &u, &Ai[a_idx - 1]);
                    ge_p1p1_to_p3_8x(&u, &t_a);
                }
            }

            // B-table selection: shared scalar digit → direct table index
            {
                int b_digit = be[d];
                if (b_digit != 0)
                {
                    int b_idx = b_digit < 0 ? -b_digit : b_digit;
                    ge_p1p1_8x t_b;
                    if (b_digit > 0)
                        ge_add_8x(&t_b, &u, &Bi[b_idx - 1]);
                    else
                        ge_sub_8x(&t_b, &u, &Bi[b_idx - 1]);

                    if (d > 0)
                        ge_p1p1_to_p2_8x(&r, &t_b);
                    else
                        ge_p1p1_to_p3_8x(&final_p3, &t_b);
                }
                else
                {
                    if (d > 0)
                    {
                        fe51x8_copy(&r.X, &u.X);
                        fe51x8_copy(&r.Y, &u.Y);
                        fe51x8_copy(&r.Z, &u.Z);
                    }
                    else
                    {
                        final_p3 = u;
                    }
                }
            }
        }

        for (size_t k = 0; k < n; k++)
            ge_p3_8x_extract(&results[batch + k], &final_p3, (int)k);

        ed25519_secure_erase(Ai, sizeof(Ai));
        ed25519_secure_erase(Bi, sizeof(Bi));
    }

    ed25519_secure_erase(ae, sizeof(ae));
    ed25519_secure_erase(be, sizeof(be));
}
