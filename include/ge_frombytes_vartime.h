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
 * @file ge_frombytes_vartime.h
 * @brief Deserialize bytes to an extended point.
 *
 * Point decompression: given 32 bytes encoding y and the sign of x,
 * recovers the full (x, y) point on the curve. This solves the curve
 * equation x^2 = (y^2 - 1) / (d*y^2 + 1) for x using fe_pow22523 and
 * fe_divpowm1, then picks the correct sign. Returns -1 if the bytes don't
 * represent a valid curve point. Rejects non-canonical y-coordinates
 * (y >= p). Variable-time because the error checks branch, which is fine
 * since the input is public (e.g., someone's public key).
 *
 * @note This function validates that the point is ON the curve, but does
 * NOT verify prime-order subgroup membership. The Ed25519 curve has
 * cofactor 8, so small-order or mixed-order points will pass this check.
 * Use ge_check_subgroup_precomp_vartime() after decoding to verify the
 * point is in the prime-order subgroup when required by the protocol
 * (e.g., Diffie-Hellman, or strict signature verification).
 */

#ifndef ED25519_GE_FROMBYTES_VARTIME_H
#define ED25519_GE_FROMBYTES_VARTIME_H

#include "ge.h"

/**
 * @brief Deserializes a 32-byte compressed point (variable-time).
 *
 * Recovers the x-coordinate from the encoded y and sign bit.
 *
 * @param h Output extended point.
 * @param s Input byte array (32 bytes).
 * @return 0 on success, -1 if the encoding is invalid.
 */
#if ED25519_PLATFORM_64BIT
int ge_frombytes_vartime_x64(ge_p3 *h, const unsigned char *s);
static inline int ge_frombytes_vartime(ge_p3 *h, const unsigned char *s)
{
    return ge_frombytes_vartime_x64(h, s);
}
#else
int ge_frombytes_vartime_portable(ge_p3 *h, const unsigned char *s);
static inline int ge_frombytes_vartime(ge_p3 *h, const unsigned char *s)
{
    return ge_frombytes_vartime_portable(h, s);
}
#endif

#endif // ED25519_GE_FROMBYTES_VARTIME_H
