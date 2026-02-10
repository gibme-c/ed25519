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
 * @file ge_fromfe_frombytes_vartime.h
 * @brief Elligator-like map from field element bytes to a curve point.
 *
 * Maps an arbitrary 32-byte string to a point on the Ed25519 curve via an
 * Elligator-style hash-to-curve construction. Unlike point decompression,
 * this accepts *any* input (not just valid encodings) and deterministically
 * produces a curve point. Useful for protocols that need to hash data to a
 * curve point without a discrete log relationship to any known point.
 */

#ifndef ED25519_GE_FROMFE_FROMBYTES_VARTIME_H
#define ED25519_GE_FROMFE_FROMBYTES_VARTIME_H

#include "ge.h"

/**
 * @brief Maps a field element (given as bytes) to a curve point (variable-time).
 *
 * Implements an Elligator-like mapping for hash-to-curve operations.
 *
 * @param r Output projective point.
 * @param s Input byte array (32 bytes representing a field element).
 */
#if ED25519_PLATFORM_64BIT
void ge_fromfe_frombytes_vartime_x64(ge_p2 *r, const unsigned char *s);
static inline void ge_fromfe_frombytes_vartime(ge_p2 *r, const unsigned char *s)
{
    ge_fromfe_frombytes_vartime_x64(r, s);
}
#else
void ge_fromfe_frombytes_vartime_portable(ge_p2 *r, const unsigned char *s);
static inline void ge_fromfe_frombytes_vartime(ge_p2 *r, const unsigned char *s)
{
    ge_fromfe_frombytes_vartime_portable(r, s);
}
#endif

#endif // ED25519_GE_FROMFE_FROMBYTES_VARTIME_H
