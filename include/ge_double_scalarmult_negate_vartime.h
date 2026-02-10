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
 * @file ge_double_scalarmult_negate_vartime.h
 * @brief Variable-time double scalar multiplication.
 *
 * Computes r = a*A + b*(-B) for arbitrary point A and base point B, using
 * the Strauss (interleaved) method. This processes both scalars a and b
 * simultaneously, sharing the doubling steps, which is almost twice as fast
 * as doing two separate scalar multiplications. Variable-time is safe here
 * because this is used in signature verification where all inputs are
 * public. Uses sliding-window decomposition (see slide.h) on both scalars
 * for further speedup.
 */

#ifndef ED25519_GE_DOUBLE_SCALARMULT_VARTIME_H
#define ED25519_GE_DOUBLE_SCALARMULT_VARTIME_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Computes r = a*A + b*B (variable-time) using precomputation table.
 *
 * @param r Output projective point.
 * @param a First 32-byte scalar.
 * @param A Precomputation table for the first point.
 * @param b Second 32-byte scalar.
 */
#if ED25519_PLATFORM_64BIT
void ge_double_scalarmult_negate_vartime_x64(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b,
    const ge_dsmp Bi);
static inline void ge_double_scalarmult_negate_vartime(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b,
    const ge_dsmp Bi)
{
#if ED25519_SIMD
    ed25519_get_dispatch().dsm_negate_vt(t, a, A, b, Bi);
#else
    ge_double_scalarmult_negate_vartime_x64(t, a, A, b, Bi);
#endif
}
#else
void ge_double_scalarmult_negate_vartime_portable(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b,
    const ge_dsmp Bi);
static inline void ge_double_scalarmult_negate_vartime(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b,
    const ge_dsmp Bi)
{
    ge_double_scalarmult_negate_vartime_portable(t, a, A, b, Bi);
}
#endif

#endif // ED25519_GE_DOUBLE_SCALARMULT_VARTIME_H
