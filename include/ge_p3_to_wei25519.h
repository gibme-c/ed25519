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
 * @file ge_p3_to_wei25519.h
 * @brief Extract the Wei25519 (short Weierstrass) X-coordinate from a ge_p3 point.
 *
 * Wei25519 is the short-Weierstrass form Y^2 = X^3 + aX + b of Curve25519
 * over GF(2^255-19). The conversion from extended Edwards coordinates is:
 *   Montgomery u = (Z+Y)/(Z-Y)
 *   Wei25519  X = u + A/3   (where A = 486662)
 *
 * The input point must not be the identity (0:1:1:0).
 */

#ifndef ED25519_GE_P3_TO_WEI25519_H
#define ED25519_GE_P3_TO_WEI25519_H

#include "ge.h"

/**
 * @brief Converts a ge_p3 point to its 32-byte Wei25519 X-coordinate.
 *
 * @param s Output byte array (32 bytes, little-endian, canonical mod p).
 * @param h Input extended point (must not be identity).
 */
#if ED25519_PLATFORM_64BIT
void ge_p3_to_wei25519_x64(unsigned char *s, const ge_p3 *h);
static inline void ge_p3_to_wei25519(unsigned char *s, const ge_p3 *h)
{
    ge_p3_to_wei25519_x64(s, h);
}
#else
void ge_p3_to_wei25519_portable(unsigned char *s, const ge_p3 *h);
static inline void ge_p3_to_wei25519(unsigned char *s, const ge_p3 *h)
{
    ge_p3_to_wei25519_portable(s, h);
}
#endif

#endif // ED25519_GE_P3_TO_WEI25519_H
