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
 * @file ge_multiscalar_mul_vartime.h
 * @brief Variable-time multi-scalar multiplication (MSM).
 *
 * Computes the combined point Q = s_0*P_0 + s_1*P_1 + ... + s_{n-1}*P_{n-1}
 * using structural sharing (shared doublings) rather than n independent
 * scalar multiplications. This gives a log_2(n) speedup factor over serial
 * computation -- roughly 6x for n=64, 10x for n=1024.
 *
 * Two algorithms are used depending on n:
 * - **Straus (interleaved)** for n <= 32: signed 4-bit fixed-window with
 *   per-point precomputed tables. Cost: 252 shared doublings + ~n*32 additions.
 * - **Pippenger (bucket method)** for n > 32: signed w-bit windows with
 *   bucket sorting. Cost: ~n*253/w additions + running-sum overhead + 253
 *   doublings. Window size w scales with log_2(n).
 *
 * Variable-time only: all MSM use cases involve public data (batch signature
 * verification, Bulletproofs, Pedersen commitment aggregation).
 *
 * The base-point variant computes base_scalar * B + MSM using the existing
 * precomputed base table, then adds the result to the MSM output.
 */

#ifndef ED25519_GE_MULTISCALAR_MUL_VARTIME_H
#define ED25519_GE_MULTISCALAR_MUL_VARTIME_H

#include "ge.h"

/**
 * @brief Multi-scalar multiplication: result = sum(scalars[i] * points[i]).
 *
 * @param result      Output extended point. Set to identity if n == 0.
 * @param scalars     Input: n x 32-byte scalars (contiguous array).
 *                    Each scalar[31] must be <= 127.
 * @param points      Input: n ge_p3 points.
 * @param n           Number of scalar-point pairs. Zero is valid (returns identity).
 */
#if ED25519_SIMD
#include "ed25519_dispatch.h"
void ge_msm_vartime_x64(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n);
static inline void
    ge_multiscalar_mul_vartime(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    ed25519_get_dispatch().msm_vartime(result, scalars, points, n);
}
#elif ED25519_PLATFORM_64BIT
void ge_msm_vartime_x64(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n);
static inline void
    ge_multiscalar_mul_vartime(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    ge_msm_vartime_x64(result, scalars, points, n);
}
#else
void ge_msm_vartime_portable(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n);
static inline void
    ge_multiscalar_mul_vartime(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n)
{
    ge_msm_vartime_portable(result, scalars, points, n);
}
#endif

/**
 * @brief Multi-scalar multiplication with base point:
 *        result = base_scalar * B + sum(scalars[i] * points[i]).
 *
 * @param result       Output extended point.
 * @param scalars      Input: n x 32-byte scalars (contiguous array).
 * @param points       Input: n ge_p3 points.
 * @param n            Number of scalar-point pairs. Zero is valid (returns base_scalar * B).
 * @param base_scalar  32-byte scalar for the base point multiplication.
 */
#if ED25519_SIMD
void ge_msm_base_vartime_x64(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar);
static inline void ge_multiscalar_mul_base_vartime(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar)
{
    ed25519_get_dispatch().msm_base_vartime(result, scalars, points, n, base_scalar);
}
#elif ED25519_PLATFORM_64BIT
void ge_msm_base_vartime_x64(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar);
static inline void ge_multiscalar_mul_base_vartime(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar)
{
    ge_msm_base_vartime_x64(result, scalars, points, n, base_scalar);
}
#else
void ge_msm_base_vartime_portable(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar);
static inline void ge_multiscalar_mul_base_vartime(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar)
{
    ge_msm_base_vartime_portable(result, scalars, points, n, base_scalar);
}
#endif

#endif // ED25519_GE_MULTISCALAR_MUL_VARTIME_H
