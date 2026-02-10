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
 * @file fe_mul.h
 * @brief Field element multiplication over GF(2^255 - 19).
 *
 * This is the performance-critical operation -- almost everything in Ed25519
 * bottlenecks on field multiplications. The algorithm multiplies all pairs of
 * limbs (schoolbook style), accumulating into 128-bit intermediates, then
 * reduces modulo p. The key trick: since p = 2^255 - 19, any overflow past
 * 2^255 wraps around as multiplication by 19, keeping the result small.
 */

#ifndef ED25519_FE_MUL_H
#define ED25519_FE_MUL_H

#include "fe.h"

/**
 * @brief Multiplies two field elements: h = f * g.
 *
 * Can overlap h with f or g.
 *
 * @param h Output field element.
 * @param f First input field element.
 * @param g Second input field element.
 */
#if ED25519_PLATFORM_64BIT
void fe_mul_x64(fe h, const fe f, const fe g);
static inline void fe_mul(fe h, const fe f, const fe g)
{
    fe_mul_x64(h, f, g);
}
#else
void fe_mul_portable(fe h, const fe f, const fe g);
static inline void fe_mul(fe h, const fe f, const fe g)
{
    fe_mul_portable(h, f, g);
}
#endif

#endif // ED25519_FE_MUL_H
