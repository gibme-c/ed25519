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
 * @file ge_scalarmult_ct_batch.h
 * @brief Batch constant-time variable-base scalar multiplication.
 *
 * Computes results[i] = scalars[i] * points[i] for i in [0, count).
 * Each operation is independent -- the batch API exists to let SIMD backends
 * process multiple scalarmults in lockstep. On x86_64, dispatches to AVX2
 * (4-way via fe10x4) or AVX-512 IFMA (8-way via fe51x8) depending on CPU
 * features. The fallback just loops over single-op ge_scalarmult_ct.
 *
 * Constant-time: the execution path depends only on count, not on scalar
 * values or point coordinates. Unused SIMD lanes (when count isn't a
 * multiple of 4 or 8) are padded with identity points and discarded.
 *
 * The output is ge_p2 (projective) rather than ge_p1p1 because the caller
 * typically needs to serialize or compare, not chain more arithmetic.
 */

#ifndef ED25519_GE_SCALARMULT_CT_BATCH_H
#define ED25519_GE_SCALARMULT_CT_BATCH_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Batch constant-time scalar multiplication.
 *
 * @param results  Output: count ge_p2 points.
 * @param scalars  Input: count x 32-byte scalars (contiguous).
 * @param points   Input: count ge_p3 points.
 * @param count    Number of operations.
 */
#if ED25519_PLATFORM_64BIT
void ge_scalarmult_ct_batch_x64(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count);
static inline void
    ge_scalarmult_ct_batch(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count)
{
#if ED25519_SIMD
    ed25519_get_dispatch().scalarmult_ct_batch(results, scalars, points, count);
#else
    ge_scalarmult_ct_batch_x64(results, scalars, points, count);
#endif
}
#else
void ge_scalarmult_ct_batch_portable(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count);
static inline void
    ge_scalarmult_ct_batch(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count)
{
    ge_scalarmult_ct_batch_portable(results, scalars, points, count);
}
#endif

#endif // ED25519_GE_SCALARMULT_CT_BATCH_H
