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
 * @file ge_double_scalarmult_negate_vartime_batch.h
 * @brief Batch variable-time double scalar multiplication with per-element scalars.
 *
 * Computes results[i] = a_scalars[i] * A_points[i] + b_scalars[i] * (-B_points[i])
 * for i in [0, count). This is a convenience batch API that loops over the single-op
 * ge_double_scalarmult_negate_vartime with ge_dsm_precomp per element.
 *
 * No SIMD dispatch: per-element scalars prevent the shared-scalar optimization
 * that makes SIMD batching profitable. The function calls through the dispatch
 * table for single-op IFMA acceleration when available.
 */

#ifndef ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_H
#define ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_H

#include "ge.h"
#include "ge_double_scalarmult_negate_vartime.h"
#include "ge_dsm_precomp.h"
#include "ge_p1p1_to_p2.h"

/**
 * @brief Batch double scalar multiplication with per-element scalars.
 *
 * @param results     Output: count ge_p2 points (projective coordinates).
 * @param a_scalars   Input: count x 32-byte scalars (contiguous array).
 * @param A_points    Input: count ge_p3 points (variable base per operation).
 * @param b_scalars   Input: count x 32-byte scalars (contiguous array).
 * @param B_points    Input: count ge_p3 points (variable base per operation).
 * @param count       Number of independent operations. Zero is a no-op.
 */
static inline void ge_double_scalarmult_negate_vartime_batch(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    const ge_p3 *B_points,
    size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        ge_dsmp Bi;
        ge_dsm_precomp(Bi, &B_points[i]);
        ge_p1p1 t;
        ge_double_scalarmult_negate_vartime(&t, a_scalars + i * 32, &A_points[i], b_scalars + i * 32, Bi);
        ge_p1p1_to_p2(&results[i], &t);
    }
}

#endif // ED25519_GE_DOUBLE_SCALARMULT_NEGATE_VARTIME_BATCH_H
