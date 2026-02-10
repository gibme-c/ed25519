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
 * @file fe_add.h
 * @brief Field element addition over GF(2^255 - 19).
 *
 * Just adds the limbs pairwise -- no carries, no reduction. This is safe
 * because the limb representation has enough headroom for several additions
 * before a multiplication (which handles carries internally) is needed.
 */

#ifndef ED25519_FE_ADD_H
#define ED25519_FE_ADD_H

#include "fe.h"

/**
 * @brief Adds two field elements: h = f + g.
 *
 * @param h Output field element.
 * @param f First input field element.
 * @param g Second input field element.
 */
#if ED25519_PLATFORM_64BIT
static inline void fe_add(fe h, const fe f, const fe g)
{
    h[0] = f[0] + g[0];
    h[1] = f[1] + g[1];
    h[2] = f[2] + g[2];
    h[3] = f[3] + g[3];
    h[4] = f[4] + g[4];
}
#else
static inline void fe_add(fe h, const fe f, const fe g)
{
    h[0] = f[0] + g[0];
    h[1] = f[1] + g[1];
    h[2] = f[2] + g[2];
    h[3] = f[3] + g[3];
    h[4] = f[4] + g[4];
    h[5] = f[5] + g[5];
    h[6] = f[6] + g[6];
    h[7] = f[7] + g[7];
    h[8] = f[8] + g[8];
    h[9] = f[9] + g[9];
}
#endif

#endif // ED25519_FE_ADD_H
