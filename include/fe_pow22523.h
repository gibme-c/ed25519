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
 * @file fe_pow22523.h
 * @brief Compute z^((p-5)/8) over GF(2^255 - 19).
 *
 * This is the core of square root computation in our field. Since
 * p mod 8 = 5, the square root of a quadratic residue u can be found as
 * u^((p+3)/8), which factors into u * u^((p-5)/8). The exponent
 * (p-5)/8 = 2^252 - 3, hence the name. Used by point decompression
 * (recovering x from the encoded y coordinate) and the Elligator map.
 */

#ifndef ED25519_FE_POW22523_H
#define ED25519_FE_POW22523_H

#include "fe.h"

/**
 * @brief Computes out = z^((2^252 - 3)), i.e., z^((p-5)/8).
 *
 * Used in square root computation for point decompression.
 *
 * @param out Output field element.
 * @param z Input field element.
 */
#if ED25519_PLATFORM_64BIT
void fe_pow22523_x64(fe out, const fe z);
static inline void fe_pow22523(fe out, const fe z)
{
    fe_pow22523_x64(out, z);
}
#else
void fe_pow22523_portable(fe out, const fe z);
static inline void fe_pow22523(fe out, const fe z)
{
    fe_pow22523_portable(out, z);
}
#endif

#endif // ED25519_FE_POW22523_H
