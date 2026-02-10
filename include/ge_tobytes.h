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
 * @file ge_tobytes.h
 * @brief Serialize a projective point to bytes.
 *
 * Compresses an elliptic curve point into 32 bytes using the standard
 * Ed25519 encoding: store the y coordinate in little-endian form and pack
 * the sign of x (its "negativity" -- see fe_isnegative) into the top bit
 * of the last byte. This works because given y and the sign of x, you can
 * always recover x from the curve equation.
 */

#ifndef ED25519_GE_TOBYTES_H
#define ED25519_GE_TOBYTES_H

#include "ge.h"

/**
 * @brief Serializes a ge_p2 point to 32-byte compressed Edwards form.
 *
 * Computes the affine y-coordinate and encodes the sign of x in the high bit.
 *
 * @param s Output byte array (32 bytes).
 * @param h Input projective point.
 */
#if ED25519_PLATFORM_64BIT
void ge_tobytes_x64(unsigned char *s, const ge_p2 *h);
static inline void ge_tobytes(unsigned char *s, const ge_p2 *h)
{
    ge_tobytes_x64(s, h);
}
#else
void ge_tobytes_portable(unsigned char *s, const ge_p2 *h);
static inline void ge_tobytes(unsigned char *s, const ge_p2 *h)
{
    ge_tobytes_portable(s, h);
}
#endif

#endif // ED25519_GE_TOBYTES_H
