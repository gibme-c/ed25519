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
 * @file ge_double_scalarmult_base_negate_vartime.h
 * @brief Variable-time double scalar multiplication with fixed base.
 *
 * Computes r = a*A + b*(-B) where B is the Ed25519 base point, using the
 * Bos-Coster/Strauss interleaved method. Same idea as
 * ge_double_scalarmult_negate_vartime but potentially uses the fixed-base
 * precomputed table for the B component. This is the main operation in
 * Ed25519 signature verification: check that s*B = R + H(R,A,M)*A.
 */

#ifndef ED25519_GE_DOUBLE_SCALARMULT_BASE_VARTIME_H
#define ED25519_GE_DOUBLE_SCALARMULT_BASE_VARTIME_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Computes r = a*A + b*B where B is the base point (variable-time).
 *
 * @param r Output projective point.
 * @param a First 32-byte scalar.
 * @param A First input extended point.
 * @param b Second 32-byte scalar.
 */
#if ED25519_PLATFORM_64BIT
void ge_double_scalarmult_base_negate_vartime_x64(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b);
static inline void
    ge_double_scalarmult_base_negate_vartime(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A, const unsigned char *b)
{
#if ED25519_SIMD
    ed25519_get_dispatch().dsm_base_negate_vt(t, a, A, b);
#else
    ge_double_scalarmult_base_negate_vartime_x64(t, a, A, b);
#endif
}
#else
void ge_double_scalarmult_base_negate_vartime_portable(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b);
static inline void
    ge_double_scalarmult_base_negate_vartime(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A, const unsigned char *b)
{
    ge_double_scalarmult_base_negate_vartime_portable(t, a, A, b);
}
#endif

#endif // ED25519_GE_DOUBLE_SCALARMULT_BASE_VARTIME_H
