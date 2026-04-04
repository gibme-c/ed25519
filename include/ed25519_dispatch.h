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
 * @file ed25519_dispatch.h
 * @brief Runtime dispatch for SIMD-accelerated scalar multiplication.
 *
 * Provides a function pointer table that routes scalar multiplication calls
 * to the fastest available implementation (x64 baseline, AVX2, or AVX-512 IFMA).
 *
 * - Without initialization: x64/portable baseline is used (always correct).
 * - After ed25519_init(): CPUID-based heuristic selects a good backend.
 * - After ed25519_init(true): per-function benchmarking selects the optimal backend.
 */

#ifndef ED25519_DISPATCH_H
#define ED25519_DISPATCH_H

#include "ed25519_platform.h"

#if ED25519_SIMD

#include "ge.h"

/**
 * @brief Function pointer table for dispatchable scalar multiplication operations.
 *
 * Each pointer targets the implementation selected by ed25519_init().
 * Initialized to x64 baseline at program startup.
 */
struct ed25519_dispatch_table
{
    void (*scalarmult_ct)(ge_p1p1 *, const unsigned char *, const ge_p3 *);
    void (*scalarmult_base_ct)(ge_p1p1 *, const unsigned char *);
    void (*dsm_base_negate_vt)(ge_p1p1 *, const unsigned char *, const ge_p3 *, const unsigned char *);
    void (*dsm_negate_vt)(ge_p1p1 *, const unsigned char *, const ge_p3 *, const unsigned char *, const ge_dsmp);
    void (*scalarmult_ct_batch)(ge_p2 *, const unsigned char *, const ge_p3 *, size_t);
    void (*dsm_base_negate_vt_batch)(ge_p2 *, const unsigned char *, const ge_p3 *, const unsigned char *, size_t);
    void (*dsm_negate_vt_batch_ss)(
        ge_p2 *,
        const unsigned char *,
        const ge_p3 *,
        const unsigned char *,
        const ge_p3 *,
        size_t);
    void (*dsm_negate_vt_batch_ss_p3)(
        ge_p3 *,
        const unsigned char *,
        const ge_p3 *,
        const unsigned char *,
        const ge_p3 *,
        size_t);
    void (*msm_vartime)(ge_p3 *, const unsigned char *, const ge_p3 *, size_t);
    void (*msm_base_vartime)(ge_p3 *, const unsigned char *, const ge_p3 *, size_t, const unsigned char *);
};

/**
 * @brief Returns a const reference to the dispatch table.
 *
 * The returned reference is read-only -- only ed25519_init() modifies
 * the internal table.
 */
const ed25519_dispatch_table &ed25519_get_dispatch();

/**
 * @brief Initialize dispatch table.
 *
 * @param autotune If false (default), uses CPUID heuristics to select a good
 *                 backend (~microseconds). If true, benchmarks all candidates
 *                 and selects the empirically fastest per function (~1-2 seconds).
 *
 * Thread-safe: concurrent callers block until the first call completes.
 * Only the first call executes; subsequent calls are no-ops regardless of
 * the autotune parameter.
 */
void ed25519_init(bool autotune = false);

#else

static inline void ed25519_init(bool = false) {}

#endif // ED25519_SIMD

#endif // ED25519_DISPATCH_H
