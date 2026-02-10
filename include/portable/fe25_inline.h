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
#ifndef ED25519_REF10_FE25_INLINE_H
#define ED25519_REF10_FE25_INLINE_H

#include "fe.h"

#if defined(_MSC_VER)
#define ED25519_FORCE_INLINE __forceinline
#elif !defined(ED25519_FORCE_INLINE)
#define ED25519_FORCE_INLINE inline __attribute__((always_inline))
#endif

static ED25519_FORCE_INLINE void fe25_mul_inline(fe h, const fe f, const fe g)
{
    int32_t f0 = f[0], f1 = f[1], f2 = f[2], f3 = f[3], f4 = f[4];
    int32_t f5 = f[5], f6 = f[6], f7 = f[7], f8 = f[8], f9 = f[9];
    int32_t g0 = g[0], g1 = g[1], g2 = g[2], g3 = g[3], g4 = g[4];
    int32_t g5 = g[5], g6 = g[6], g7 = g[7], g8 = g[8], g9 = g[9];
    int32_t g1_19 = 19 * g1, g2_19 = 19 * g2, g3_19 = 19 * g3;
    int32_t g4_19 = 19 * g4, g5_19 = 19 * g5, g6_19 = 19 * g6;
    int32_t g7_19 = 19 * g7, g8_19 = 19 * g8, g9_19 = 19 * g9;
    int32_t f1_2 = 2 * f1, f3_2 = 2 * f3, f5_2 = 2 * f5, f7_2 = 2 * f7, f9_2 = 2 * f9;

    int64_t h0 = f0 * (int64_t)g0 + f1_2 * (int64_t)g9_19 + f2 * (int64_t)g8_19 + f3_2 * (int64_t)g7_19
                 + f4 * (int64_t)g6_19 + f5_2 * (int64_t)g5_19 + f6 * (int64_t)g4_19 + f7_2 * (int64_t)g3_19
                 + f8 * (int64_t)g2_19 + f9_2 * (int64_t)g1_19;
    int64_t h1 = f0 * (int64_t)g1 + f1 * (int64_t)g0 + f2 * (int64_t)g9_19 + f3 * (int64_t)g8_19 + f4 * (int64_t)g7_19
                 + f5 * (int64_t)g6_19 + f6 * (int64_t)g5_19 + f7 * (int64_t)g4_19 + f8 * (int64_t)g3_19
                 + f9 * (int64_t)g2_19;
    int64_t h2 = f0 * (int64_t)g2 + f1_2 * (int64_t)g1 + f2 * (int64_t)g0 + f3_2 * (int64_t)g9_19 + f4 * (int64_t)g8_19
                 + f5_2 * (int64_t)g7_19 + f6 * (int64_t)g6_19 + f7_2 * (int64_t)g5_19 + f8 * (int64_t)g4_19
                 + f9_2 * (int64_t)g3_19;
    int64_t h3 = f0 * (int64_t)g3 + f1 * (int64_t)g2 + f2 * (int64_t)g1 + f3 * (int64_t)g0 + f4 * (int64_t)g9_19
                 + f5 * (int64_t)g8_19 + f6 * (int64_t)g7_19 + f7 * (int64_t)g6_19 + f8 * (int64_t)g5_19
                 + f9 * (int64_t)g4_19;
    int64_t h4 = f0 * (int64_t)g4 + f1_2 * (int64_t)g3 + f2 * (int64_t)g2 + f3_2 * (int64_t)g1 + f4 * (int64_t)g0
                 + f5_2 * (int64_t)g9_19 + f6 * (int64_t)g8_19 + f7_2 * (int64_t)g7_19 + f8 * (int64_t)g6_19
                 + f9_2 * (int64_t)g5_19;
    int64_t h5 = f0 * (int64_t)g5 + f1 * (int64_t)g4 + f2 * (int64_t)g3 + f3 * (int64_t)g2 + f4 * (int64_t)g1
                 + f5 * (int64_t)g0 + f6 * (int64_t)g9_19 + f7 * (int64_t)g8_19 + f8 * (int64_t)g7_19
                 + f9 * (int64_t)g6_19;
    int64_t h6 = f0 * (int64_t)g6 + f1_2 * (int64_t)g5 + f2 * (int64_t)g4 + f3_2 * (int64_t)g3 + f4 * (int64_t)g2
                 + f5_2 * (int64_t)g1 + f6 * (int64_t)g0 + f7_2 * (int64_t)g9_19 + f8 * (int64_t)g8_19
                 + f9_2 * (int64_t)g7_19;
    int64_t h7 = f0 * (int64_t)g7 + f1 * (int64_t)g6 + f2 * (int64_t)g5 + f3 * (int64_t)g4 + f4 * (int64_t)g3
                 + f5 * (int64_t)g2 + f6 * (int64_t)g1 + f7 * (int64_t)g0 + f8 * (int64_t)g9_19 + f9 * (int64_t)g8_19;
    int64_t h8 = f0 * (int64_t)g8 + f1_2 * (int64_t)g7 + f2 * (int64_t)g6 + f3_2 * (int64_t)g5 + f4 * (int64_t)g4
                 + f5_2 * (int64_t)g3 + f6 * (int64_t)g2 + f7_2 * (int64_t)g1 + f8 * (int64_t)g0
                 + f9_2 * (int64_t)g9_19;
    int64_t h9 = f0 * (int64_t)g9 + f1 * (int64_t)g8 + f2 * (int64_t)g7 + f3 * (int64_t)g6 + f4 * (int64_t)g5
                 + f5 * (int64_t)g4 + f6 * (int64_t)g3 + f7 * (int64_t)g2 + f8 * (int64_t)g1 + f9 * (int64_t)g0;

    int64_t carry0, carry1, carry2, carry3, carry4, carry5, carry6, carry7, carry8, carry9;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry1 = (h1 + (int64_t)(1 << 24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry5 = (h5 + (int64_t)(1 << 24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;
    carry2 = (h2 + (int64_t)(1 << 25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry6 = (h6 + (int64_t)(1 << 25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;
    carry3 = (h3 + (int64_t)(1 << 24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry7 = (h7 + (int64_t)(1 << 24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry8 = (h8 + (int64_t)(1 << 25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;
    carry9 = (h9 + (int64_t)(1 << 24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;
    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;

    h[0] = (int32_t)h0;
    h[1] = (int32_t)h1;
    h[2] = (int32_t)h2;
    h[3] = (int32_t)h3;
    h[4] = (int32_t)h4;
    h[5] = (int32_t)h5;
    h[6] = (int32_t)h6;
    h[7] = (int32_t)h7;
    h[8] = (int32_t)h8;
    h[9] = (int32_t)h9;
}

static ED25519_FORCE_INLINE void fe25_sq_inline(fe h, const fe f)
{
    int32_t f0 = f[0], f1 = f[1], f2 = f[2], f3 = f[3], f4 = f[4];
    int32_t f5 = f[5], f6 = f[6], f7 = f[7], f8 = f[8], f9 = f[9];
    int32_t f0_2 = 2 * f0, f1_2 = 2 * f1, f2_2 = 2 * f2, f3_2 = 2 * f3, f4_2 = 2 * f4;
    int32_t f5_2 = 2 * f5, f6_2 = 2 * f6, f7_2 = 2 * f7;
    int32_t f5_38 = 38 * f5, f6_19 = 19 * f6, f7_38 = 38 * f7, f8_19 = 19 * f8, f9_38 = 38 * f9;

    int64_t h0 = f0 * (int64_t)f0 + f1_2 * (int64_t)f9_38 + f2_2 * (int64_t)f8_19 + f3_2 * (int64_t)f7_38
                 + f4_2 * (int64_t)f6_19 + f5 * (int64_t)f5_38;
    int64_t h1 =
        f0_2 * (int64_t)f1 + f2 * (int64_t)f9_38 + f3_2 * (int64_t)f8_19 + f4 * (int64_t)f7_38 + f5_2 * (int64_t)f6_19;
    int64_t h2 = f0_2 * (int64_t)f2 + f1_2 * (int64_t)f1 + f3_2 * (int64_t)f9_38 + f4_2 * (int64_t)f8_19
                 + f5_2 * (int64_t)f7_38 + f6 * (int64_t)f6_19;
    int64_t h3 =
        f0_2 * (int64_t)f3 + f1_2 * (int64_t)f2 + f4 * (int64_t)f9_38 + f5_2 * (int64_t)f8_19 + f6 * (int64_t)f7_38;
    int64_t h4 = f0_2 * (int64_t)f4 + f1_2 * (int64_t)f3_2 + f2 * (int64_t)f2 + f5_2 * (int64_t)f9_38
                 + f6_2 * (int64_t)f8_19 + f7 * (int64_t)f7_38;
    int64_t h5 =
        f0_2 * (int64_t)f5 + f1_2 * (int64_t)f4 + f2_2 * (int64_t)f3 + f6 * (int64_t)f9_38 + f7_2 * (int64_t)f8_19;
    int64_t h6 = f0_2 * (int64_t)f6 + f1_2 * (int64_t)f5_2 + f2_2 * (int64_t)f4 + f3_2 * (int64_t)f3
                 + f7_2 * (int64_t)f9_38 + f8 * (int64_t)f8_19;
    int64_t h7 =
        f0_2 * (int64_t)f7 + f1_2 * (int64_t)f6 + f2_2 * (int64_t)f5 + f3_2 * (int64_t)f4 + f8 * (int64_t)f9_38;
    int64_t h8 = f0_2 * (int64_t)f8 + f1_2 * (int64_t)f7_2 + f2_2 * (int64_t)f6 + f3_2 * (int64_t)f5_2
                 + f4 * (int64_t)f4 + f9 * (int64_t)f9_38;
    int64_t h9 = f0_2 * (int64_t)f9 + f1_2 * (int64_t)f8 + f2_2 * (int64_t)f7 + f3_2 * (int64_t)f6 + f4_2 * (int64_t)f5;

    int64_t carry0, carry1, carry2, carry3, carry4, carry5, carry6, carry7, carry8, carry9;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry1 = (h1 + (int64_t)(1 << 24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry5 = (h5 + (int64_t)(1 << 24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;
    carry2 = (h2 + (int64_t)(1 << 25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry6 = (h6 + (int64_t)(1 << 25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;
    carry3 = (h3 + (int64_t)(1 << 24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry7 = (h7 + (int64_t)(1 << 24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry8 = (h8 + (int64_t)(1 << 25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;
    carry9 = (h9 + (int64_t)(1 << 24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;
    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;

    h[0] = (int32_t)h0;
    h[1] = (int32_t)h1;
    h[2] = (int32_t)h2;
    h[3] = (int32_t)h3;
    h[4] = (int32_t)h4;
    h[5] = (int32_t)h5;
    h[6] = (int32_t)h6;
    h[7] = (int32_t)h7;
    h[8] = (int32_t)h8;
    h[9] = (int32_t)h9;
}

#endif // ED25519_REF10_FE25_INLINE_H
