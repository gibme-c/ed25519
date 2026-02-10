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
 * @file fe_neg.h
 * @brief Field element negation over GF(2^255 - 19).
 *
 * Computes h = -f = p - f. Same bias-and-carry approach as fe_sub on 64-bit,
 * simple sign flip on portable.
 */

#ifndef ED25519_FE_NEG_H
#define ED25519_FE_NEG_H

#include "fe.h"

/**
 * @brief Negates a field element: h = -f.
 *
 * @param h Output field element.
 * @param f Input field element.
 */
#if ED25519_PLATFORM_64BIT
#include "x64/fe51.h"
static inline void fe_neg(fe h, const fe f)
{
    uint64_t c;
    h[0] = 0xFFFFFFFFFFFDAULL - f[0];
    c = h[0] >> 51;
    h[0] &= FE51_MASK;
    h[1] = 0xFFFFFFFFFFFFEULL - f[1] + c;
    c = h[1] >> 51;
    h[1] &= FE51_MASK;
    h[2] = 0xFFFFFFFFFFFFEULL - f[2] + c;
    c = h[2] >> 51;
    h[2] &= FE51_MASK;
    h[3] = 0xFFFFFFFFFFFFEULL - f[3] + c;
    c = h[3] >> 51;
    h[3] &= FE51_MASK;
    h[4] = 0xFFFFFFFFFFFFEULL - f[4] + c;
    c = h[4] >> 51;
    h[4] &= FE51_MASK;
    h[0] += c * 19;
}
#else
static inline void fe_neg(fe h, const fe f)
{
    h[0] = -f[0];
    h[1] = -f[1];
    h[2] = -f[2];
    h[3] = -f[3];
    h[4] = -f[4];
    h[5] = -f[5];
    h[6] = -f[6];
    h[7] = -f[7];
    h[8] = -f[8];
    h[9] = -f[9];
}
#endif

#endif // ED25519_FE_NEG_H
