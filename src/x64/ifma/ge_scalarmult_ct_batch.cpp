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
 * @file ifma/ge_scalarmult_ct_batch.cpp
 * @brief IFMA 8-way batch constant-time variable-base scalar multiplication.
 *
 * Processes 8 independent scalarmult operations in parallel using fe51x8
 * field arithmetic with AVX-512 IFMA. Same algorithm as the AVX2 4-way
 * batch but with stride 8 and radix-2^51 field elements.
 */

#include "ed25519_secure_erase.h"
#include "ge.h"
#include "ge_p1p1_to_p2.h"
#include "x64/ifma/ge_batch_ifma.h"

// d2 = 2*d constant in fe51
static const fe ge_batch_ifma_d2_fe51 =
    {0x69b9426b2f159ULL, 0x35050762add7aULL, 0x3cf44c0038052ULL, 0x6738cc7407977ULL, 0x2406d9dc56dffULL};

void ge_scalarmult_ct_batch_ifma(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count)
{
    if (count == 0)
        return;

    // Broadcast d2 constant into all 8 lanes
    fe51x8 d2_8x;
    fe51x8_broadcast_d2(&d2_8x, ge_batch_ifma_d2_fe51);

    for (size_t batch = 0; batch < count; batch += 8)
    {
        size_t n = (count - batch < 8) ? (count - batch) : 8;

        // Encode scalars into signed 4-bit digits
        alignas(64) signed char e[8][64];
        for (size_t k = 0; k < 8; k++)
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
                for (int i = 0; i < 64; i++)
                    e[k][i] = 0;
            }
        }

        // Pack up to 8 points into ge_p3_8x (padding with identity)
        ge_p3_8x A8x;
        ge_p3_0_8x(&A8x);
        for (size_t k = 0; k < n; k++)
            ge_p3_8x_insert(&A8x, &points[batch + k], (int)k);

        // Build Ai[8] table: Ai[j] = (j+1) * A for j=0..7
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

        // Main loop: process 4-bit digits from MSB to LSB
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

            // Per-lane table selection using k-mask compare
            __m512i digits = _mm512_set_epi64(
                (long long)e[7][d],
                (long long)e[6][d],
                (long long)e[5][d],
                (long long)e[4][d],
                (long long)e[3][d],
                (long long)e[2][d],
                (long long)e[1][d],
                (long long)e[0][d]);

            // neg_mask: lanes where digit < 0 (avoid _mm512_movepi64_mask which needs AVX-512DQ)
            const __m512i zero = _mm512_setzero_si512();
            __mmask8 neg_mask = _mm512_cmpgt_epi64_mask(zero, digits);
            // abs_digits = (digits XOR sign) - sign
            __m512i sign = _mm512_srai_epi64(digits, 63);
            __m512i abs_digits = _mm512_sub_epi64(_mm512_xor_si512(digits, sign), sign);

            ge_cached_8x cur;
            ge_cached_0_8x(&cur);

            for (int j = 1; j <= 8; j++)
            {
                __mmask8 eq = _mm512_cmpeq_epi64_mask(abs_digits, _mm512_set1_epi64(j));
                ge_cached_cmov_8x(&cur, &Ai[j - 1], eq);
            }

            ge_cached_cneg_8x(&cur, neg_mask);
            ge_add_8x(&t, &u, &cur);
            ge_p1p1_to_p2_8x(&r, &t);
        }

        // Extract results
        for (size_t k = 0; k < n; k++)
            ge_p2_8x_extract(&results[batch + k], &r, (int)k);

        // Secure erase temporaries
        ed25519_secure_erase(e, sizeof(e));
        ed25519_secure_erase(&Ai, sizeof(Ai));
        ed25519_secure_erase(&u, sizeof(u));
        ed25519_secure_erase(&r, sizeof(r));
        ed25519_secure_erase(&t, sizeof(t));
    }
}
