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
 * @file fe_invert.h
 * @brief Field element multiplicative inverse over GF(2^255 - 19).
 *
 * Computes 1/z using Fermat's little theorem: in a prime field, z^(p-1) = 1,
 * so z^(p-2) = 1/z. Since p-2 = 2^255 - 21, we can compute this with a
 * carefully chosen sequence of squarings and multiplications called an
 * "addition chain." The chain requires about 254 squarings and 11
 * multiplications -- expensive, which is exactly why projective coordinates
 * exist (they batch all the inversions to the very end).
 */

#ifndef ED25519_FE_INVERT_H
#define ED25519_FE_INVERT_H

#include "fe.h"

/**
 * @brief Computes the multiplicative inverse: out = z^(p-2) mod p.
 *
 * Uses Fermat's little theorem with an addition chain.
 *
 * @param out Output field element (the inverse).
 * @param z Input field element.
 */
#if ED25519_PLATFORM_64BIT
void fe_invert_x64(fe out, const fe z);
static inline void fe_invert(fe out, const fe z)
{
    fe_invert_x64(out, z);
}
#else
void fe_invert_portable(fe out, const fe z);
static inline void fe_invert(fe out, const fe z)
{
    fe_invert_portable(out, z);
}
#endif

#endif // ED25519_FE_INVERT_H
