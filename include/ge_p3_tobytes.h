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
 * @file ge_p3_tobytes.h
 * @brief Serialize an extended point to bytes.
 *
 * Same compressed encoding as ge_tobytes but starting from the extended
 * (ge_p3) representation. Since both p2 and p3 share X, Y, Z coordinates,
 * the serialization logic is the same: divide by Z to get affine, encode y,
 * and stash the sign of x in the high bit.
 */

#ifndef ED25519_GE_P3_TOBYTES_H
#define ED25519_GE_P3_TOBYTES_H

#include "ge.h"

/**
 * @brief Serializes a ge_p3 point to 32-byte compressed Edwards form.
 *
 * Computes the affine y-coordinate and encodes the sign of x in the high bit.
 *
 * @param s Output byte array (32 bytes).
 * @param h Input extended point.
 */
#if ED25519_PLATFORM_64BIT
void ge_p3_tobytes_x64(unsigned char *s, const ge_p3 *h);
static inline void ge_p3_tobytes(unsigned char *s, const ge_p3 *h)
{
    ge_p3_tobytes_x64(s, h);
}
#else
void ge_p3_tobytes_portable(unsigned char *s, const ge_p3 *h);
static inline void ge_p3_tobytes(unsigned char *s, const ge_p3 *h)
{
    ge_p3_tobytes_portable(s, h);
}
#endif

#endif // ED25519_GE_P3_TOBYTES_H
