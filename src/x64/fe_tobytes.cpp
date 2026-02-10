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
#include "x64/fe_tobytes.h"

#include "x64/fe51.h"

void fe_tobytes_x64(unsigned char *s, const fe h)
{
    uint64_t t[5];
    t[0] = h[0];
    t[1] = h[1];
    t[2] = h[2];
    t[3] = h[3];
    t[4] = h[4];

    // First carry normalization pass: reduce all limbs to ~51 bits
    uint64_t carry;
    carry = t[0] >> 51;
    t[1] += carry;
    t[0] &= FE51_MASK;
    carry = t[1] >> 51;
    t[2] += carry;
    t[1] &= FE51_MASK;
    carry = t[2] >> 51;
    t[3] += carry;
    t[2] &= FE51_MASK;
    carry = t[3] >> 51;
    t[4] += carry;
    t[3] &= FE51_MASK;
    carry = t[4] >> 51;
    t[0] += carry * 19;
    t[4] &= FE51_MASK;
    // Second carry for the wraparound
    carry = t[0] >> 51;
    t[1] += carry;
    t[0] &= FE51_MASK;

    // Canonical reduction: compute q = floor((value + 19) / 2^255)
    // If q=1, value >= p, so subtract p by adding 19.
    uint64_t q = (t[0] + 19) >> 51;
    q = (t[1] + q) >> 51;
    q = (t[2] + q) >> 51;
    q = (t[3] + q) >> 51;
    q = (t[4] + q) >> 51;

    // Now q is 0 or 1. If 1, we need to subtract p.
    t[0] += 19 * q;

    // Final carry chain
    carry = t[0] >> 51;
    t[1] += carry;
    t[0] &= FE51_MASK;
    carry = t[1] >> 51;
    t[2] += carry;
    t[1] &= FE51_MASK;
    carry = t[2] >> 51;
    t[3] += carry;
    t[2] &= FE51_MASK;
    carry = t[3] >> 51;
    t[4] += carry;
    t[3] &= FE51_MASK;
    t[4] &= FE51_MASK;

    // Serialize 5 x 51-bit limbs to 32 bytes little-endian
    s[0] = (unsigned char)(t[0]);
    s[1] = (unsigned char)(t[0] >> 8);
    s[2] = (unsigned char)(t[0] >> 16);
    s[3] = (unsigned char)(t[0] >> 24);
    s[4] = (unsigned char)(t[0] >> 32);
    s[5] = (unsigned char)(t[0] >> 40);
    s[6] = (unsigned char)((t[0] >> 48) | (t[1] << 3));
    s[7] = (unsigned char)(t[1] >> 5);
    s[8] = (unsigned char)(t[1] >> 13);
    s[9] = (unsigned char)(t[1] >> 21);
    s[10] = (unsigned char)(t[1] >> 29);
    s[11] = (unsigned char)(t[1] >> 37);
    s[12] = (unsigned char)((t[1] >> 45) | (t[2] << 6));
    s[13] = (unsigned char)(t[2] >> 2);
    s[14] = (unsigned char)(t[2] >> 10);
    s[15] = (unsigned char)(t[2] >> 18);
    s[16] = (unsigned char)(t[2] >> 26);
    s[17] = (unsigned char)(t[2] >> 34);
    s[18] = (unsigned char)(t[2] >> 42);
    s[19] = (unsigned char)((t[2] >> 50) | (t[3] << 1));
    s[20] = (unsigned char)(t[3] >> 7);
    s[21] = (unsigned char)(t[3] >> 15);
    s[22] = (unsigned char)(t[3] >> 23);
    s[23] = (unsigned char)(t[3] >> 31);
    s[24] = (unsigned char)(t[3] >> 39);
    s[25] = (unsigned char)((t[3] >> 47) | (t[4] << 4));
    s[26] = (unsigned char)(t[4] >> 4);
    s[27] = (unsigned char)(t[4] >> 12);
    s[28] = (unsigned char)(t[4] >> 20);
    s[29] = (unsigned char)(t[4] >> 28);
    s[30] = (unsigned char)(t[4] >> 36);
    s[31] = (unsigned char)(t[4] >> 44);
}
