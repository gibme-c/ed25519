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
 * @file dispatch.cpp
 * @brief Runtime dispatch table and auto-tuning for SIMD scalar multiplication.
 *
 * Implements the dispatch table, CPUID-based initialization, and
 * benchmarking-based auto-tuning for the six dispatchable scalar multiplication
 * operations.
 */

#include "ed25519_dispatch.h"

#if ED25519_SIMD

#include "ed25519_cpuid.h"
#include "ge_dsm_precomp.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "sc_clamp.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <vector>

// ── Forward declarations of all implementation functions ──
// These are defined in their respective TUs (src/x64/, src/x64/avx2/, src/x64/ifma/).

// x64 baseline (always available on 64-bit)
void ge_scalarmult_x64_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A);
void ge_scalarmult_base_ct_x64(ge_p1p1 *r, const unsigned char *a);
void ge_double_scalarmult_base_negate_vartime_x64(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b);
void ge_double_scalarmult_negate_vartime_x64(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b,
    const ge_dsmp Bi);
void ge_scalarmult_ct_batch_x64(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count);
void ge_dsm_base_negate_vt_batch_x64(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count);
void ge_dsm_negate_vt_batch_ss_x64(
    ge_p2 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
void ge_dsm_negate_vt_batch_ss_p3_x64(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
void ge_msm_vartime_x64(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n);
void ge_msm_base_vartime_x64(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar);

// AVX2 (compiled when ENABLE_AVX2=ON)
#if !ED25519_NO_AVX2
void ge_scalarmult_avx2_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A);
void ge_scalarmult_base_ct_avx2(ge_p1p1 *r, const unsigned char *a);
void ge_scalarmult_ct_batch_avx2(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count);
void ge_dsm_base_negate_vt_batch_avx2(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count);
void ge_dsm_negate_vt_batch_ss_avx2(
    ge_p2 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
void ge_dsm_negate_vt_batch_ss_p3_avx2(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
void ge_msm_vartime_avx2(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n);
void ge_msm_base_vartime_avx2(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar);
#endif

// IFMA (compiled when ENABLE_AVX512=ON, all 4 functions have IFMA variants)
#if !ED25519_NO_AVX512
void ge_scalarmult_ifma_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A);
void ge_scalarmult_base_ct_ifma(ge_p1p1 *r, const unsigned char *a);
void ge_double_scalarmult_base_negate_vartime_ifma(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b);
void ge_double_scalarmult_negate_vartime_ifma(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b,
    const ge_dsmp Bi);
void ge_scalarmult_ct_batch_ifma(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count);
void ge_dsm_base_negate_vt_batch_ifma(
    ge_p2 *results,
    const unsigned char *a_scalars,
    const ge_p3 *A_points,
    const unsigned char *b_scalars,
    size_t count);
void ge_dsm_negate_vt_batch_ss_ifma(
    ge_p2 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
void ge_dsm_negate_vt_batch_ss_p3_ifma(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count);
void ge_msm_vartime_ifma(ge_p3 *result, const unsigned char *scalars, const ge_p3 *points, size_t n);
void ge_msm_base_vartime_ifma(
    ge_p3 *result,
    const unsigned char *scalars,
    const ge_p3 *points,
    size_t n,
    const unsigned char *base_scalar);
#endif

// ── File-local dispatch table — initialized to x64 baseline ──
static ed25519_dispatch_table dispatch_table = {
    ge_scalarmult_x64_ct,
    ge_scalarmult_base_ct_x64,
    ge_double_scalarmult_base_negate_vartime_x64,
    ge_double_scalarmult_negate_vartime_x64,
    ge_scalarmult_ct_batch_x64,
    ge_dsm_base_negate_vt_batch_x64,
    ge_dsm_negate_vt_batch_ss_x64,
    ge_dsm_negate_vt_batch_ss_p3_x64,
    ge_msm_vartime_x64,
    ge_msm_base_vartime_x64,
};

const ed25519_dispatch_table &ed25519_get_dispatch()
{
    return dispatch_table;
}

// ── CPUID-based heuristic initialization ──

static std::once_flag init_flag;
static std::once_flag autotune_flag;

void ed25519_init(void)
{
    std::call_once(
        init_flag,
        []()
        {
            // Reset to x64 baseline
            dispatch_table.scalarmult_ct = ge_scalarmult_x64_ct;
            dispatch_table.scalarmult_base_ct = ge_scalarmult_base_ct_x64;
            dispatch_table.dsm_base_negate_vt = ge_double_scalarmult_base_negate_vartime_x64;
            dispatch_table.dsm_negate_vt = ge_double_scalarmult_negate_vartime_x64;
            dispatch_table.scalarmult_ct_batch = ge_scalarmult_ct_batch_x64;
            dispatch_table.dsm_base_negate_vt_batch = ge_dsm_base_negate_vt_batch_x64;
            dispatch_table.dsm_negate_vt_batch_ss = ge_dsm_negate_vt_batch_ss_x64;
            dispatch_table.dsm_negate_vt_batch_ss_p3 = ge_dsm_negate_vt_batch_ss_p3_x64;
            dispatch_table.msm_vartime = ge_msm_vartime_x64;
            dispatch_table.msm_base_vartime = ge_msm_base_vartime_x64;

            const uint32_t features = ed25519_cpu_features();

        // IFMA is the fastest overall backend when available (especially on MSVC
        // where it avoids uint128 struct emulation, and after lazy normalization
        // eliminated the overhead that previously penalized GCC).
#if !ED25519_NO_AVX512
            if (features & ED25519_CPU_AVX512IFMA)
            {
                dispatch_table.scalarmult_ct = ge_scalarmult_ifma_ct;
                dispatch_table.scalarmult_base_ct = ge_scalarmult_base_ct_ifma;
                dispatch_table.dsm_base_negate_vt = ge_double_scalarmult_base_negate_vartime_ifma;
                dispatch_table.dsm_negate_vt = ge_double_scalarmult_negate_vartime_ifma;
                dispatch_table.scalarmult_ct_batch = ge_scalarmult_ct_batch_ifma;
                dispatch_table.dsm_base_negate_vt_batch = ge_dsm_base_negate_vt_batch_ifma;
                dispatch_table.dsm_negate_vt_batch_ss = ge_dsm_negate_vt_batch_ss_ifma;
                dispatch_table.dsm_negate_vt_batch_ss_p3 = ge_dsm_negate_vt_batch_ss_p3_ifma;
                dispatch_table.msm_vartime = ge_msm_vartime_ifma;
                dispatch_table.msm_base_vartime = ge_msm_base_vartime_ifma;
                return;
            }
#endif

        // AVX2 available for CT functions and batch operations.
#if !ED25519_NO_AVX2
            if (features & ED25519_CPU_AVX2)
            {
                dispatch_table.scalarmult_ct = ge_scalarmult_avx2_ct;
                dispatch_table.scalarmult_base_ct = ge_scalarmult_base_ct_avx2;
                dispatch_table.scalarmult_ct_batch = ge_scalarmult_ct_batch_avx2;
                dispatch_table.dsm_base_negate_vt_batch = ge_dsm_base_negate_vt_batch_avx2;
                dispatch_table.dsm_negate_vt_batch_ss = ge_dsm_negate_vt_batch_ss_avx2;
                dispatch_table.dsm_negate_vt_batch_ss_p3 = ge_dsm_negate_vt_batch_ss_p3_avx2;
                dispatch_table.msm_vartime = ge_msm_vartime_avx2;
                dispatch_table.msm_base_vartime = ge_msm_base_vartime_avx2;
            }
#endif

            (void)features;
        });
}

// ── Auto-tune implementation ──

namespace
{

    using hrclock = std::chrono::high_resolution_clock;
    using ns = std::chrono::nanoseconds;

    constexpr int TUNE_WARMUP = 8;
    constexpr int TUNE_ITERS = 32;

    static int64_t bench_scalarmult_ct(
        void (*fn)(ge_p1p1 *, const unsigned char *, const ge_p3 *),
        const unsigned char *scalar,
        const ge_p3 *point)
    {
        ge_p1p1 result;
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(&result, scalar, point);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(&result, scalar, point);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_scalarmult_base_ct(void (*fn)(ge_p1p1 *, const unsigned char *), const unsigned char *scalar)
    {
        ge_p1p1 result;
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(&result, scalar);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(&result, scalar);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_dsm_base_negate_vt(
        void (*fn)(ge_p1p1 *, const unsigned char *, const ge_p3 *, const unsigned char *),
        const unsigned char *scalar_a,
        const ge_p3 *point,
        const unsigned char *scalar_b)
    {
        ge_p1p1 result;
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(&result, scalar_a, point, scalar_b);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(&result, scalar_a, point, scalar_b);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_dsm_negate_vt(
        void (*fn)(ge_p1p1 *, const unsigned char *, const ge_p3 *, const unsigned char *, const ge_dsmp),
        const unsigned char *scalar_a,
        const ge_p3 *point,
        const unsigned char *scalar_b,
        const ge_dsmp Bi)
    {
        ge_p1p1 result;
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(&result, scalar_a, point, scalar_b, Bi);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(&result, scalar_a, point, scalar_b, Bi);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_scalarmult_ct_batch(
        void (*fn)(ge_p2 *, const unsigned char *, const ge_p3 *, size_t),
        const unsigned char *scalars,
        const ge_p3 *points,
        size_t count)
    {
        std::vector<ge_p2> results(count);
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(results.data(), scalars, points, count);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(results.data(), scalars, points, count);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_dsm_base_negate_vt_batch(
        void (*fn)(ge_p2 *, const unsigned char *, const ge_p3 *, const unsigned char *, size_t),
        const unsigned char *a_scalars,
        const ge_p3 *points,
        const unsigned char *b_scalars,
        size_t count)
    {
        std::vector<ge_p2> results(count);
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(results.data(), a_scalars, points, b_scalars, count);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(results.data(), a_scalars, points, b_scalars, count);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_dsm_negate_vt_batch_ss(
        void (*fn)(ge_p2 *, const unsigned char *, const ge_p3 *, const unsigned char *, const ge_p3 *, size_t),
        const unsigned char *a,
        const ge_p3 *A_points,
        const unsigned char *b,
        const ge_p3 *B_points,
        size_t count)
    {
        std::vector<ge_p2> results(count);
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(results.data(), a, A_points, b, B_points, count);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(results.data(), a, A_points, b, B_points, count);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_dsm_negate_vt_batch_ss_p3(
        void (*fn)(ge_p3 *, const unsigned char *, const ge_p3 *, const unsigned char *, const ge_p3 *, size_t),
        const unsigned char *a,
        const ge_p3 *A_points,
        const unsigned char *b,
        const ge_p3 *B_points,
        size_t count)
    {
        std::vector<ge_p3> results(count);
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(results.data(), a, A_points, b, B_points, count);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(results.data(), a, A_points, b, B_points, count);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

    static int64_t bench_msm_vartime(
        void (*fn)(ge_p3 *, const unsigned char *, const ge_p3 *, size_t),
        const unsigned char *scalars,
        const ge_p3 *points,
        size_t count)
    {
        ge_p3 result;
        for (int i = 0; i < TUNE_WARMUP; i++)
            fn(&result, scalars, points, count);

        int64_t best = INT64_MAX;
        for (int i = 0; i < TUNE_ITERS; i++)
        {
            auto start = hrclock::now();
            fn(&result, scalars, points, count);
            auto elapsed = std::chrono::duration_cast<ns>(hrclock::now() - start).count();
            if (elapsed < best)
                best = elapsed;
        }
        return best;
    }

} // anonymous namespace

void ed25519_autotune(void)
{
    std::call_once(
        autotune_flag,
        []()
        {
            const uint32_t features = ed25519_cpu_features();

            // Generate test inputs using x64 baseline (avoids circular dispatch dependency)
            unsigned char s1[32], s2[32];
            for (int i = 0; i < 32; i++)
            {
                s1[i] = static_cast<unsigned char>(i + 1);
                s2[i] = static_cast<unsigned char>(i + 65);
            }
            sc_clamp(s1);
            sc_clamp(s2);

            ge_p1p1 tmp;
            ge_p3 test_point;
            ge_scalarmult_base_ct_x64(&tmp, s1);
            ge_p1p1_to_p3(&test_point, &tmp);

            ge_dsmp test_dsmp;
            ge_dsm_precomp(test_dsmp, &test_point);

            // ── ge_scalarmult_ct ──
            {
                int64_t best_time = bench_scalarmult_ct(ge_scalarmult_x64_ct, s1, &test_point);
                decltype(dispatch_table.scalarmult_ct) best_fn = ge_scalarmult_x64_ct;

#if !ED25519_NO_AVX2
                if (features & ED25519_CPU_AVX2)
                {
                    auto t = bench_scalarmult_ct(ge_scalarmult_avx2_ct, s1, &test_point);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_scalarmult_avx2_ct;
                    }
                }
#endif
#if !ED25519_NO_AVX512
                if (features & ED25519_CPU_AVX512IFMA)
                {
                    auto t = bench_scalarmult_ct(ge_scalarmult_ifma_ct, s1, &test_point);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_scalarmult_ifma_ct;
                    }
                }
#endif
                dispatch_table.scalarmult_ct = best_fn;
            }

            // ── ge_scalarmult_base_ct ──
            {
                int64_t best_time = bench_scalarmult_base_ct(ge_scalarmult_base_ct_x64, s1);
                decltype(dispatch_table.scalarmult_base_ct) best_fn = ge_scalarmult_base_ct_x64;

#if !ED25519_NO_AVX2
                if (features & ED25519_CPU_AVX2)
                {
                    auto t = bench_scalarmult_base_ct(ge_scalarmult_base_ct_avx2, s1);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_scalarmult_base_ct_avx2;
                    }
                }
#endif
#if !ED25519_NO_AVX512
                if (features & ED25519_CPU_AVX512IFMA)
                {
                    auto t = bench_scalarmult_base_ct(ge_scalarmult_base_ct_ifma, s1);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_scalarmult_base_ct_ifma;
                    }
                }
#endif
                dispatch_table.scalarmult_base_ct = best_fn;
            }

            // ── ge_double_scalarmult_base_negate_vartime ──
            {
                int64_t best_time =
                    bench_dsm_base_negate_vt(ge_double_scalarmult_base_negate_vartime_x64, s1, &test_point, s2);
                decltype(dispatch_table.dsm_base_negate_vt) best_fn = ge_double_scalarmult_base_negate_vartime_x64;

#if !ED25519_NO_AVX512
                if (features & ED25519_CPU_AVX512IFMA)
                {
                    auto t =
                        bench_dsm_base_negate_vt(ge_double_scalarmult_base_negate_vartime_ifma, s1, &test_point, s2);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_double_scalarmult_base_negate_vartime_ifma;
                    }
                }
#endif
                dispatch_table.dsm_base_negate_vt = best_fn;
            }

            // ── ge_double_scalarmult_negate_vartime ──
            {
                int64_t best_time =
                    bench_dsm_negate_vt(ge_double_scalarmult_negate_vartime_x64, s1, &test_point, s2, test_dsmp);
                decltype(dispatch_table.dsm_negate_vt) best_fn = ge_double_scalarmult_negate_vartime_x64;

#if !ED25519_NO_AVX512
                if (features & ED25519_CPU_AVX512IFMA)
                {
                    auto t =
                        bench_dsm_negate_vt(ge_double_scalarmult_negate_vartime_ifma, s1, &test_point, s2, test_dsmp);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_double_scalarmult_negate_vartime_ifma;
                    }
                }
#endif
                dispatch_table.dsm_negate_vt = best_fn;
            }

            // ── Batch operations: ge_scalarmult_ct_batch ──
            {
                constexpr size_t BATCH_N = 16;
                unsigned char batch_scalars[BATCH_N * 32];
                ge_p3 batch_points[BATCH_N];
                for (size_t i = 0; i < BATCH_N; i++)
                {
                    for (int j = 0; j < 32; j++)
                        batch_scalars[i * 32 + j] = static_cast<unsigned char>(j + i + 1);
                    sc_clamp(batch_scalars + i * 32);
                    ge_p1p1 tmp2;
                    ge_scalarmult_base_ct_x64(&tmp2, batch_scalars + i * 32);
                    ge_p1p1_to_p3(&batch_points[i], &tmp2);
                }

                int64_t best_time =
                    bench_scalarmult_ct_batch(ge_scalarmult_ct_batch_x64, batch_scalars, batch_points, BATCH_N);
                decltype(dispatch_table.scalarmult_ct_batch) best_fn = ge_scalarmult_ct_batch_x64;

#if !ED25519_NO_AVX2
                if (features & ED25519_CPU_AVX2)
                {
                    auto t =
                        bench_scalarmult_ct_batch(ge_scalarmult_ct_batch_avx2, batch_scalars, batch_points, BATCH_N);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_scalarmult_ct_batch_avx2;
                    }
                }
#endif
#if !ED25519_NO_AVX512
                if (features & ED25519_CPU_AVX512IFMA)
                {
                    auto t =
                        bench_scalarmult_ct_batch(ge_scalarmult_ct_batch_ifma, batch_scalars, batch_points, BATCH_N);
                    if (t < best_time)
                    {
                        best_time = t;
                        best_fn = ge_scalarmult_ct_batch_ifma;
                    }
                }
#endif
                dispatch_table.scalarmult_ct_batch = best_fn;

                // ── Batch DSM ──
                {
                    unsigned char batch_b_scalars[BATCH_N * 32];
                    for (size_t i = 0; i < BATCH_N; i++)
                    {
                        for (int j = 0; j < 32; j++)
                            batch_b_scalars[i * 32 + j] = static_cast<unsigned char>(j + i + 65);
                        sc_clamp(batch_b_scalars + i * 32);
                    }

                    int64_t best_time2 = bench_dsm_base_negate_vt_batch(
                        ge_dsm_base_negate_vt_batch_x64, batch_scalars, batch_points, batch_b_scalars, BATCH_N);
                    decltype(dispatch_table.dsm_base_negate_vt_batch) best_fn2 = ge_dsm_base_negate_vt_batch_x64;

#if !ED25519_NO_AVX2
                    if (features & ED25519_CPU_AVX2)
                    {
                        auto t = bench_dsm_base_negate_vt_batch(
                            ge_dsm_base_negate_vt_batch_avx2, batch_scalars, batch_points, batch_b_scalars, BATCH_N);
                        if (t < best_time2)
                        {
                            best_time2 = t;
                            best_fn2 = ge_dsm_base_negate_vt_batch_avx2;
                        }
                    }
#endif
#if !ED25519_NO_AVX512
                    if (features & ED25519_CPU_AVX512IFMA)
                    {
                        auto t = bench_dsm_base_negate_vt_batch(
                            ge_dsm_base_negate_vt_batch_ifma, batch_scalars, batch_points, batch_b_scalars, BATCH_N);
                        if (t < best_time2)
                        {
                            best_time2 = t;
                            best_fn2 = ge_dsm_base_negate_vt_batch_ifma;
                        }
                    }
#endif
                    dispatch_table.dsm_base_negate_vt_batch = best_fn2;
                }

                // ── Batch DSM shared-scalar ──
                {
                    // Reuse batch_points as both A and B points for tuning
                    int64_t best_time3 = bench_dsm_negate_vt_batch_ss(
                        ge_dsm_negate_vt_batch_ss_x64, s1, batch_points, s2, batch_points, BATCH_N);
                    decltype(dispatch_table.dsm_negate_vt_batch_ss) best_fn3 = ge_dsm_negate_vt_batch_ss_x64;

#if !ED25519_NO_AVX2
                    if (features & ED25519_CPU_AVX2)
                    {
                        auto t = bench_dsm_negate_vt_batch_ss(
                            ge_dsm_negate_vt_batch_ss_avx2, s1, batch_points, s2, batch_points, BATCH_N);
                        if (t < best_time3)
                        {
                            best_time3 = t;
                            best_fn3 = ge_dsm_negate_vt_batch_ss_avx2;
                        }
                    }
#endif
#if !ED25519_NO_AVX512
                    if (features & ED25519_CPU_AVX512IFMA)
                    {
                        auto t = bench_dsm_negate_vt_batch_ss(
                            ge_dsm_negate_vt_batch_ss_ifma, s1, batch_points, s2, batch_points, BATCH_N);
                        if (t < best_time3)
                        {
                            best_time3 = t;
                            best_fn3 = ge_dsm_negate_vt_batch_ss_ifma;
                        }
                    }
#endif
                    dispatch_table.dsm_negate_vt_batch_ss = best_fn3;
                }

                // ── Batch DSM shared-scalar p3 ──
                {
                    int64_t best_time4 = bench_dsm_negate_vt_batch_ss_p3(
                        ge_dsm_negate_vt_batch_ss_p3_x64, s1, batch_points, s2, batch_points, BATCH_N);
                    decltype(dispatch_table.dsm_negate_vt_batch_ss_p3) best_fn4 = ge_dsm_negate_vt_batch_ss_p3_x64;

#if !ED25519_NO_AVX2
                    if (features & ED25519_CPU_AVX2)
                    {
                        auto t = bench_dsm_negate_vt_batch_ss_p3(
                            ge_dsm_negate_vt_batch_ss_p3_avx2, s1, batch_points, s2, batch_points, BATCH_N);
                        if (t < best_time4)
                        {
                            best_time4 = t;
                            best_fn4 = ge_dsm_negate_vt_batch_ss_p3_avx2;
                        }
                    }
#endif
#if !ED25519_NO_AVX512
                    if (features & ED25519_CPU_AVX512IFMA)
                    {
                        auto t = bench_dsm_negate_vt_batch_ss_p3(
                            ge_dsm_negate_vt_batch_ss_p3_ifma, s1, batch_points, s2, batch_points, BATCH_N);
                        if (t < best_time4)
                        {
                            best_time4 = t;
                            best_fn4 = ge_dsm_negate_vt_batch_ss_p3_ifma;
                        }
                    }
#endif
                    dispatch_table.dsm_negate_vt_batch_ss_p3 = best_fn4;
                }

                // ── MSM (multi-scalar multiplication) ──
                {
                    // Use the batch points already generated (BATCH_N=16, within Straus range)
                    int64_t best_time5 = bench_msm_vartime(ge_msm_vartime_x64, batch_scalars, batch_points, BATCH_N);
                    decltype(dispatch_table.msm_vartime) best_fn5 = ge_msm_vartime_x64;

#if !ED25519_NO_AVX2
                    if (features & ED25519_CPU_AVX2)
                    {
                        auto t = bench_msm_vartime(ge_msm_vartime_avx2, batch_scalars, batch_points, BATCH_N);
                        if (t < best_time5)
                        {
                            best_time5 = t;
                            best_fn5 = ge_msm_vartime_avx2;
                        }
                    }
#endif
#if !ED25519_NO_AVX512
                    if (features & ED25519_CPU_AVX512IFMA)
                    {
                        auto t = bench_msm_vartime(ge_msm_vartime_ifma, batch_scalars, batch_points, BATCH_N);
                        if (t < best_time5)
                        {
                            best_time5 = t;
                            best_fn5 = ge_msm_vartime_ifma;
                        }
                    }
#endif
                    dispatch_table.msm_vartime = best_fn5;

                    // msm_base_vartime: delegates to msm_vartime internally, so use the same selection
                    // (the base point part is handled by ge_scalarmult_base_ct which has its own dispatch)
                    if (best_fn5 == ge_msm_vartime_x64)
                        dispatch_table.msm_base_vartime = ge_msm_base_vartime_x64;
#if !ED25519_NO_AVX2
                    else if (best_fn5 == ge_msm_vartime_avx2)
                        dispatch_table.msm_base_vartime = ge_msm_base_vartime_avx2;
#endif
#if !ED25519_NO_AVX512
                    else if (best_fn5 == ge_msm_vartime_ifma)
                        dispatch_table.msm_base_vartime = ge_msm_base_vartime_ifma;
#endif
                }
            }

            (void)features;
        });
}

#endif // ED25519_SIMD
