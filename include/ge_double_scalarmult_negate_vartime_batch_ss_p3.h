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
 * @file ge_double_scalarmult_negate_vartime_batch_ss_p3.h
 * @brief Batch variable-time double scalar multiplication with shared scalars, returning ge_p3.
 *
 * Identical to ge_double_scalarmult_negate_vartime_batch_ss except the output
 * is ge_p3 (extended coordinates with T = X*Y/Z) instead of ge_p2 (projective).
 *
 * This avoids the expensive ge_p2_to_p3 conversion (fe_invert + 3 fe_mul per
 * point) that callers would otherwise need when feeding results into the next
 * round -- e.g. Bulletproofs IPA folding where each round's output points
 * become the next round's input. The cost is just one extra field multiply per
 * point (p1p1_to_p3 vs p1p1_to_p2 on the final iteration).
 */

#ifndef ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_SS_P3_H
#define ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_SS_P3_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Batch double scalar multiplication with shared scalars, returning ge_p3.
 *
 * @param results     Output: count ge_p3 points (extended coordinates).
 * @param a           Input: single 32-byte scalar applied to all A_points.
 * @param A_points    Input: count ge_p3 points (variable base per operation).
 * @param b           Input: single 32-byte scalar applied to all B_points.
 * @param B_points    Input: count ge_p3 points (variable base per operation).
 * @param count       Number of independent operations. Zero is a no-op.
 */
#if ED25519_PLATFORM_64BIT
void ge_dsm_negate_vt_batch_ss_p3_x64(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
static inline void ge_double_scalarmult_negate_vartime_batch_ss_p3(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count)
{
#if ED25519_SIMD
    ed25519_get_dispatch().dsm_negate_vt_batch_ss_p3(results, a, A_points, b, B_points, count);
#else
    ge_dsm_negate_vt_batch_ss_p3_x64(results, a, A_points, b, B_points, count);
#endif
}
#else
void ge_dsm_negate_vt_batch_ss_p3_portable(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
static inline void ge_double_scalarmult_negate_vartime_batch_ss_p3(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count)
{
    ge_dsm_negate_vt_batch_ss_p3_portable(results, a, A_points, b, B_points, count);
}
#endif

#endif // ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_SS_P3_H
