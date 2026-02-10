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
 * @file ge_double_scalarmult_negate_vartime_batch_ss.h
 * @brief Batch variable-time double scalar multiplication with shared scalars.
 *
 * Computes results[i] = a * A_points[i] + b * (-B_points[i]) for i in [0, count),
 * where a single scalar pair (a, b) is shared across all operations.
 *
 * The shared-scalar property enables a critical SIMD optimization: since all
 * lanes process the same scalar digits, table lookups use direct indexing
 * instead of per-lane conditional moves (8-iteration cmov loops), eliminating
 * the dominant source of SIMD overhead. This makes the batch genuinely faster
 * than serial for workloads like Bulletproofs IPA folding where the same
 * challenge scalar pair is applied to many point pairs.
 *
 * On x86_64 with SIMD enabled, dispatches through the ed25519_dispatch table
 * to AVX2 (4-way via fe10x4) or AVX-512 IFMA (8-way via fe51x8). The
 * fallback just loops over single-op ge_double_scalarmult_negate_vartime.
 */

#ifndef ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_SS_H
#define ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_SS_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Batch double scalar multiplication with shared scalars.
 *
 * @param results     Output: count ge_p2 points (projective coordinates).
 * @param a           Input: single 32-byte scalar applied to all A_points.
 * @param A_points    Input: count ge_p3 points (variable base per operation).
 * @param b           Input: single 32-byte scalar applied to all B_points.
 * @param B_points    Input: count ge_p3 points (variable base per operation).
 * @param count       Number of independent operations. Zero is a no-op.
 */
#if ED25519_PLATFORM_64BIT
void ge_dsm_negate_vt_batch_ss_x64(
    ge_p2 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
static inline void ge_double_scalarmult_negate_vartime_batch_ss(
    ge_p2 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count)
{
#if ED25519_SIMD
    ed25519_get_dispatch().dsm_negate_vt_batch_ss(results, a, A_points, b, B_points, count);
#else
    ge_dsm_negate_vt_batch_ss_x64(results, a, A_points, b, B_points, count);
#endif
}
#else
void ge_dsm_negate_vt_batch_ss_portable(
    ge_p2 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
static inline void ge_double_scalarmult_negate_vartime_batch_ss(
    ge_p2 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count)
{
    ge_dsm_negate_vt_batch_ss_portable(results, a, A_points, b, B_points, count);
}
#endif

#endif // ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_SS_H
