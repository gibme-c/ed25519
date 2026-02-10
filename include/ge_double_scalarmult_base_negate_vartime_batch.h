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
 * @file ge_double_scalarmult_base_negate_vartime_batch.h
 * @brief Batch variable-time double scalar multiplication with fixed base.
 *
 * Computes results[i] = a_scalars[i] * A_points[i] + b_scalars[i] * (-B)
 * for i in [0, count), where B is the Ed25519 base point. This is the batch
 * version of ge_double_scalarmult_base_negate_vartime -- the core operation
 * in Ed25519 signature verification. The batch API lets SIMD backends process
 * multiple independent verifications in lockstep.
 *
 * On x86_64 with SIMD enabled, dispatches through the ed25519_dispatch table
 * to AVX2 (4-way via fe10x4) or AVX-512 IFMA (8-way via fe51x8). The
 * fallback just loops over single-op ge_double_scalarmult_base_negate_vartime.
 *
 * Variable-time: the execution path depends on scalar values, so this must
 * only be used on public data (verification, not signing). Unused SIMD lanes
 * are padded with identity points and zero scalars, then discarded.
 *
 * Both batch operations use a 4-bit fixed-window encoding with a lookup table
 * of consecutive multiples {1B, 2B, ..., 8B} built on the fly from the base
 * point. This is different from the single-op vartime version, which uses odd
 * multiples for a sliding-window approach -- the fixed-window encoding is more
 * SIMD-friendly since all lanes step through digits in lockstep.
 */

#ifndef ED25519_GE_DOUBLE_SCALARMULT_BASE_NEGATE_VARTIME_BATCH_H
#define ED25519_GE_DOUBLE_SCALARMULT_BASE_NEGATE_VARTIME_BATCH_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Batch double scalar multiplication for verification.
 *
 * @param results    Output: count ge_p2 points (projective coordinates).
 * @param a_scalars  Input: count x 32-byte scalars (contiguous array).
 * @param A_points   Input: count ge_p3 points (variable base per operation).
 * @param b_scalars  Input: count x 32-byte scalars (contiguous array).
 * @param count      Number of independent operations. Zero is a no-op.
 */
#if ED25519_PLATFORM_64BIT
void ge_dsm_base_negate_vt_batch_x64(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count);
static inline void ge_double_scalarmult_base_negate_vartime_batch(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count)
{
#if ED25519_SIMD
    ed25519_get_dispatch().dsm_base_negate_vt_batch(results, a_scalars, A_points, b_scalars, count);
#else
    ge_dsm_base_negate_vt_batch_x64(results, a_scalars, A_points, b_scalars, count);
#endif
}
#else
void ge_dsm_base_negate_vt_batch_portable(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count);
static inline void ge_double_scalarmult_base_negate_vartime_batch(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count)
{
    ge_dsm_base_negate_vt_batch_portable(results, a_scalars, A_points, b_scalars, count);
}
#endif

#endif // ED25519_GE_DOUBLE_SCALARMULT_BASE_NEGATE_VARTIME_BATCH_H
