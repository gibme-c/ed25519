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
 * @file fe_sq.h
 * @brief Field element squaring over GF(2^255 - 19).
 *
 * Squaring is a specialized multiplication where both inputs are the same.
 * This lets us nearly halve the number of limb-pair multiplications by
 * doubling the cross-terms (since a[i]*a[j] appears twice). It's worth
 * having a dedicated squaring function because exponentiation chains (used
 * for inversion and square roots) do hundreds of squarings in a row.
 */

#ifndef ED25519_FE_SQ_H
#define ED25519_FE_SQ_H

#include "fe.h"

/**
 * @brief Squares a field element: h = f^2.
 *
 * @param h Output field element.
 * @param f Input field element.
 */
#if ED25519_PLATFORM_64BIT
void fe_sq_x64(fe h, const fe f);
static inline void fe_sq(fe h, const fe f)
{
    fe_sq_x64(h, f);
}
#else
void fe_sq_portable(fe h, const fe f);
static inline void fe_sq(fe h, const fe f)
{
    fe_sq_portable(h, f);
}
#endif

#endif // ED25519_FE_SQ_H
