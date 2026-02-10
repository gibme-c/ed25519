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
#include "x64/fe_sq2.h"

#include "x64/fe51.h"
#include "x64/mul128.h"

void fe_sq2_x64(fe h, const fe f)
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

#if ED25519_HAVE_INT128
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

#elif ED25519_HAVE_UMUL128
    ed25519_uint128_emu h0 = mul64(f0, f0);
    h0 += mul64(f1_38, f4);
    h0 += mul64(f2_19, f3_2);

    ed25519_uint128_emu h1 = mul64(f0_2, f1);
    h1 += mul64(f2_38, f4);
    h1 += mul64(f3_19, f3);

    ed25519_uint128_emu h2 = mul64(f0_2, f2);
    h2 += mul64(f1, f1);
    h2 += mul64(f3_38, f4);

    ed25519_uint128_emu h3 = mul64(f0_2, f3);
    h3 += mul64(f1_2, f2);
    h3 += mul64(f4_19, f4);

    ed25519_uint128_emu h4 = mul64(f0_2, f4);
    h4 += mul64(f1_2, f3);
    h4 += mul64(f2, f2);

    h0 += h0;
    h1 += h1;
    h2 += h2;
    h3 += h3;
    h4 += h4;

    uint64_t carry;
    carry = shr128(h0, 51);
    h1 += carry;
    uint64_t r0 = lo128(h0) & FE51_MASK;
    carry = shr128(h1, 51);
    h2 += carry;
    uint64_t r1 = lo128(h1) & FE51_MASK;
    carry = shr128(h2, 51);
    h3 += carry;
    uint64_t r2 = lo128(h2) & FE51_MASK;
    carry = shr128(h3, 51);
    h4 += carry;
    uint64_t r3 = lo128(h3) & FE51_MASK;
    carry = shr128(h4, 51);
    r0 += carry * 19;
    uint64_t r4 = lo128(h4) & FE51_MASK;
    carry = r0 >> 51;
    r1 += carry;
    r0 &= FE51_MASK;

    h[0] = r0;
    h[1] = r1;
    h[2] = r2;
    h[3] = r3;
    h[4] = r4;
#endif
}
