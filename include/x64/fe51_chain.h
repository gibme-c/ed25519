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
#ifndef ED25519_X64_FE51_CHAIN_H
#define ED25519_X64_FE51_CHAIN_H

/*
 * Chain operation primitives for fe_invert, fe_pow22523, fe_divpowm1.
 *
 * GCC/Clang: force-inlined fe51 mul/sq for maximum throughput.
 * MSVC: regular fe_mul_x64/fe_sq_x64 calls — MSVC's optimizer chokes on the
 *       massive inlined bodies created by the struct-based ed25519_uint128_emu,
 *       causing severe register spilling and 2-5x regressions.
 *       For batch squaring, MSVC uses fe_sqn_x64 which keeps limbs in locals
 *       across the loop, eliminating per-call ABI overhead.
 */

#if defined(_MSC_VER)

#include "fe.h"

void fe_mul_x64(fe h, const fe f, const fe g);
void fe_sq_x64(fe h, const fe f);
void fe_sq2_x64(fe h, const fe f);
void fe_sqn_x64(fe h, const fe f, int n);

#define fe51_chain_mul fe_mul_x64
#define fe51_chain_sq fe_sq_x64
#define fe51_chain_sq2 fe_sq2_x64
#define fe51_chain_sqn fe_sqn_x64

#else

#include "x64/fe51_inline.h"

#define fe51_chain_mul fe51_mul_inline
#define fe51_chain_sq fe51_sq_inline

static ED25519_FORCE_INLINE void fe51_sq2_inline(fe h, const fe f)
{
    uint64_t f0 = f[0], f1 = f[1], f2 = f[2], f3 = f[3], f4 = f[4];

    uint64_t f0_2 = 2 * f0;
    uint64_t f1_2 = 2 * f1;
    uint64_t f3_2 = 2 * f3;

    uint64_t f1_38 = 38 * f1;
    uint64_t f2_19 = 19 * f2;
    uint64_t f2_38 = 38 * f2;
    uint64_t f3_19 = 19 * f3;
    uint64_t f3_38 = 38 * f3;
    uint64_t f4_19 = 19 * f4;

    ed25519_uint128 h0 = mul64(f0, f0) + mul64(f1_38, f4) + mul64(f2_19, f3_2);
    ed25519_uint128 h1 = mul64(f0_2, f1) + mul64(f2_38, f4) + mul64(f3_19, f3);
    ed25519_uint128 h2 = mul64(f0_2, f2) + mul64(f1, f1) + mul64(f3_38, f4);
    ed25519_uint128 h3 = mul64(f0_2, f3) + mul64(f1_2, f2) + mul64(f4_19, f4);
    ed25519_uint128 h4 = mul64(f0_2, f4) + mul64(f1_2, f3) + mul64(f2, f2);

    h0 += h0;
    h1 += h1;
    h2 += h2;
    h3 += h3;
    h4 += h4;

    uint64_t carry;
    carry = (uint64_t)(h0 >> 51);
    h1 += carry;
    h0 &= FE51_MASK;
    carry = (uint64_t)(h1 >> 51);
    h2 += carry;
    h1 &= FE51_MASK;
    carry = (uint64_t)(h2 >> 51);
    h3 += carry;
    h2 &= FE51_MASK;
    carry = (uint64_t)(h3 >> 51);
    h4 += carry;
    h3 &= FE51_MASK;
    carry = (uint64_t)(h4 >> 51);
    h0 += carry * 19;
    h4 &= FE51_MASK;
    carry = (uint64_t)(h0 >> 51);
    h1 += carry;
    h0 &= FE51_MASK;

    h[0] = (uint64_t)h0;
    h[1] = (uint64_t)h1;
    h[2] = (uint64_t)h2;
    h[3] = (uint64_t)h3;
    h[4] = (uint64_t)h4;
}

#define fe51_chain_sq2 fe51_sq2_inline

static ED25519_FORCE_INLINE void fe51_sqn_inline(fe h, const fe f, int n)
{
    fe51_sq_inline(h, f);
    for (int i = 1; i < n; i++)
        fe51_sq_inline(h, h);
}

#define fe51_chain_sqn fe51_sqn_inline

#endif

#endif // ED25519_X64_FE51_CHAIN_H
