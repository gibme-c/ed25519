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
 * @file benchmark.cpp
 * @brief Benchmark driver for Ed25519 library operations.
 */

#include "ed25519.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int main(int argc, char *argv[])
{
    const auto bench_state = benchmark_setup();

    // Parse CLI options
    const char *dispatch_label = "baseline (x64/portable)";
    bool bench_batch = false;
    bool bench_msm = false;
    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--autotune") == 0)
        {
            ed25519_init(true);
            dispatch_label = "autotune";
        }
        else if (std::strcmp(argv[i], "--init") == 0)
        {
            ed25519_init();
            dispatch_label = "init (CPUID heuristic)";
        }
        else if (std::strcmp(argv[i], "--batch") == 0)
        {
            bench_batch = true;
        }
        else if (std::strcmp(argv[i], "--msm") == 0)
        {
            bench_msm = true;
        }
        else
        {
            std::cerr << "Usage: " << argv[0] << " [--init | --autotune] [--batch] [--msm]" << std::endl;
            return 1;
        }
    }

    std::cout << "Benchmark Timings" << std::endl;
    std::cout << "Dispatch: " << dispatch_label << std::endl;
    std::cout << std::endl;

#if ED25519_SIMD
    {
        const uint32_t features = ed25519_cpu_features();

        std::cout << "CPU features:";
        std::cout << " AVX2=" << ((features & ED25519_CPU_AVX2) ? "yes" : "no");
        std::cout << " AVX-512F=" << ((features & ED25519_CPU_AVX512F) ? "yes" : "no");
        std::cout << " AVX-512-IFMA=" << ((features & ED25519_CPU_AVX512IFMA) ? "yes" : "no");
        std::cout << std::endl << std::endl;
    }
#endif

    const uint8_t G[32] = {0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66};

    const uint8_t H[32] = {0xdd, 0x2a, 0xf5, 0xc2, 0x8a, 0xcc, 0xdc, 0x50, 0xc8, 0xbc, 0x4e,
                           0x15, 0x99, 0x12, 0x82, 0x3a, 0x87, 0x87, 0xc1, 0x18, 0x52, 0x97,
                           0x74, 0x5f, 0xb2, 0x30, 0xe2, 0x64, 0x6c, 0xd7, 0x7e, 0xf6};

    const uint8_t scalar[32] = {0x31, 0x3b, 0x08, 0x3f, 0x84, 0x28, 0x2b, 0x00, 0xb9, 0xc8, 0x4f,
                                0x4c, 0xf4, 0x39, 0x24, 0xf6, 0x61, 0x27, 0xf5, 0xd2, 0x77, 0x2f,
                                0xdf, 0x36, 0x11, 0x09, 0x56, 0xa8, 0xda, 0xd5, 0x98, 0x04};

    if (sc_check_reduced(reinterpret_cast<const unsigned char *>(&scalar)) != 0)
    {
        std::cout << "Invalid scalar detection in test scalar" << std::endl;

        return 1;
    }

    if (sc_check_reduced(reinterpret_cast<const unsigned char *>(&G)) == 0)
    {
        std::cout << "Invalid scalar detection in test point" << std::endl;

        return 1;
    }

    ge_p3 G_point3, H_point3;

    ge_p2 G_point2, H_point2;

    ge_cached G_cached, H_cached;

    ge_p1p1 G_p1p1, H_p1p1;

    if (ge_frombytes_vartime(&G_point3, G) != 0)
    {
        std::fprintf(stderr, "benchmark: ge_frombytes_vartime(G) failed\n");
        std::abort();
    }

    if (ge_frombytes_vartime(&H_point3, H) != 0)
    {
        std::fprintf(stderr, "benchmark: ge_frombytes_vartime(H) failed\n");
        std::abort();
    }

    ge_fromfe_frombytes_vartime(&G_point2, G);

    ge_fromfe_frombytes_vartime(&H_point2, H);

    ge_p3_to_cached(&G_cached, &G_point3);

    ge_p3_to_cached(&H_cached, &H_point3);

    ge_add(&G_p1p1, &G_point3, &G_cached);

    ge_add(&H_p1p1, &H_point3, &H_cached);

    ge_precomp G_precomp;

    ge_precomp_0(&G_precomp);

    ge_dsmp G_dsmp;

    ge_dsm_precomp(G_dsmp, &G_point3);


    // =========================================================================
    // Section 1: Field Operations
    // =========================================================================

    std::cout << "Field Operations" << std::endl << std::endl;

    benchmark_header();

#if ED25519_PLATFORM_64BIT
    const fe a = {0x34dca135978a3ULL, 0x1a8283b156ebdULL, 0x5e7a26001c029ULL, 0x739c663a03cbbULL, 0x52036cee2b6ffULL};
#else
    const fe a = {-10913610, 13857413, -15372611, 6949391, 114729, -8787816, -6275908, -3247719, -18696448, -12055116};
#endif

    benchmark(
        []()
        {
            fe h;
            fe_0(h);
        },
        "fe_0");

    benchmark(
        []()
        {
            fe h;
            fe_1(h);
        },
        "fe_1");

    benchmark(
        [&a]()
        {
            fe h;
            fe_copy(h, a);
        },
        "fe_copy");

    benchmark(
        [&a]()
        {
            fe f;
            fe_copy(f, a);
            fe_cmov(f, a, 1);
        },
        "fe_cmov");

    benchmark(
        [&G]()
        {
            fe h;
            fe_frombytes(h, G);
        },
        "fe_frombytes");

    benchmark(
        [&a]()
        {
            unsigned char bytes[32] = {0};
            fe_tobytes(bytes, a);
        },
        "fe_tobytes");

    benchmark(
        [&a]()
        {
            fe b;
            fe_add(b, a, a);
        },
        "fe_add");

    benchmark(
        [&a]()
        {
            fe b;
            fe_sub(b, a, a);
        },
        "fe_sub");

    benchmark(
        [&a]()
        {
            fe b;
            fe_neg(b, a);
        },
        "fe_neg");

    benchmark(
        [&a]()
        {
            fe b;
            fe_mul(b, a, a);
        },
        "fe_mul");

    benchmark(
        [&a]()
        {
            fe b;
            fe_sq(b, a);
        },
        "fe_sq");

    benchmark(
        [&a]()
        {
            fe b;
            fe_sq2(b, a);
        },
        "fe_sq2");

    benchmark(
        [&a]()
        {
            fe b;
            fe_divpowm1(b, a, a);
        },
        "fe_divpowm1");

    benchmark(
        [&a]()
        {
            fe b;
            fe_pow22523(b, a);
        },
        "fe_pow22523");

    benchmark(
        [&a]()
        {
            fe b;
            fe_invert(b, a);
        },
        "fe_invert");

    benchmark([&a]() { benchmark_do_not_optimize(fe_isnegative(a)); }, "fe_isnegative");

    benchmark([&a]() { benchmark_do_not_optimize(fe_isnonzero(a)); }, "fe_isnonzero");

    // =========================================================================
    // Section 2: Group Element Serialization
    // =========================================================================

    if (H_point3 == G_point3 || H_point2 == G_point2 || H_cached == G_cached || H_p1p1 == G_p1p1)
    {
        std::cout << "Invalid point comparison" << std::endl;

        return 1;
    }

    std::cout << std::endl << "Group Element Serialization" << std::endl << std::endl;

    benchmark_header();

    benchmark(
        [&G]()
        {
            ge_p3 point;

            if (ge_frombytes_vartime(&point, G) != 0)
            {
                std::fprintf(stderr, "benchmark: ge_frombytes_vartime failed on G\n");
                std::abort();
            }
        },
        "ge_frombytes_vartime");

    benchmark(
        [&G]()
        {
            ge_p2 point;

            ge_fromfe_frombytes_vartime(&point, G);
        },
        "ge_fromfe_frombytes_vartime");

    benchmark(
        [&G_point3]()
        {
            uint8_t bytes[32] = {0};

            ge_p3_tobytes(reinterpret_cast<unsigned char *>(&bytes), &G_point3);
        },
        "ge_p3_tobytes");

    benchmark(
        [&G_point2]()
        {
            uint8_t bytes[32] = {0};

            ge_tobytes(reinterpret_cast<unsigned char *>(&bytes), &G_point2);
        },
        "ge_tobytes");

    benchmark(
        [&G_point3]()
        {
            uint8_t bytes[32] = {0};

            ge_p3_to_wei25519(reinterpret_cast<unsigned char *>(&bytes), &G_point3);
        },
        "ge_p3_to_wei25519");

    // =========================================================================
    // Section 3: Group Element Conversions
    // =========================================================================

    std::cout << std::endl << "Group Element Conversions" << std::endl << std::endl;

    benchmark_header();

    benchmark(
        []()
        {
            ge_p2 h;
            ge_p2_0(&h);
        },
        "ge_p2_0");

    benchmark(
        []()
        {
            ge_p3 h;
            ge_p3_0(&h);
        },
        "ge_p3_0");

    benchmark(
        []()
        {
            ge_cached h;
            ge_cached_0(&h);
        },
        "ge_cached_0");

    benchmark(
        []()
        {
            ge_precomp h;
            ge_precomp_0(&h);
        },
        "ge_precomp_0");

    benchmark(
        [&G_point3]()
        {
            ge_p2 point;

            ge_p3_to_p2(&point, &G_point3);
        },
        "ge_p3_to_p2");

    benchmark(
        [&G_point3]()
        {
            ge_cached point;

            ge_p3_to_cached(&point, &G_point3);
        },
        "ge_p3_to_cached");

    benchmark(
        [&G_point2]()
        {
            ge_p3 point;

            ge_p2_to_p3(&point, &G_point2);
        },
        "ge_p2_to_p3");

    benchmark(
        [&G_p1p1]()
        {
            ge_p2 point;

            ge_p1p1_to_p2(&point, &G_p1p1);
        },
        "ge_p1p1_to_p2");

    benchmark(
        [&G_p1p1]()
        {
            ge_p3 point;

            ge_p1p1_to_p3(&point, &G_p1p1);
        },
        "ge_p1p1_to_p3");

    benchmark(
        [&G_cached]()
        {
            ge_cached t;
            ge_cached_cmov(&t, &G_cached, 1);
        },
        "ge_cached_cmov");

    benchmark(
        [&G_precomp]()
        {
            ge_precomp t;
            ge_precomp_cmov(&t, &G_precomp, 1);
        },
        "ge_precomp_cmov");

    // =========================================================================
    // Section 4: Group Element Arithmetic
    // =========================================================================

    std::cout << std::endl << "Group Element Arithmetic" << std::endl << std::endl;

    benchmark_header();

    benchmark(
        [&G_point3, &G_cached]()
        {
            ge_p1p1 point;

            ge_add(&point, &G_point3, &G_cached);
        },
        "ge_add");

    benchmark(
        [&G_point3, &G_cached]()
        {
            ge_p1p1 point;

            ge_sub(&point, &G_point3, &G_cached);
        },
        "ge_sub");

    benchmark(
        [&G_point3, &G_precomp]()
        {
            ge_p1p1 r;

            ge_madd(&r, &G_point3, &G_precomp);
        },
        "ge_madd");

    benchmark(
        [&G_point3, &G_precomp]()
        {
            ge_p1p1 r;

            ge_msub(&r, &G_point3, &G_precomp);
        },
        "ge_msub");

    benchmark(
        [&G_point2]()
        {
            ge_p1p1 point;

            ge_p2_dbl(&point, &G_point2);
        },
        "ge_p2_dbl");

    benchmark(
        [&G_point3]()
        {
            ge_p1p1 point;

            ge_p3_dbl(&point, &G_point3);
        },
        "ge_p3_dbl");

    benchmark(
        [&G_point2]()
        {
            ge_p1p1 point;

            ge_mul8(&point, &G_point2);
        },
        "ge_mul8");

    // =========================================================================
    // Section 5: Scalar Multiplication
    // =========================================================================

    std::cout << std::endl << "Scalar Multiplication" << std::endl << std::endl;

    benchmark_header();

    benchmark(
        [&G_point3]()
        {
            ge_dsmp point;

            ge_dsm_precomp(point, &G_point3);
        },
        "ge_dsm_precomp");

    benchmark(
        [&G_dsmp]() { benchmark_do_not_optimize(ge_check_subgroup_precomp_negate_vartime(G_dsmp)); },
        "ge_check_subgroup_precomp_negate_vartime");

    benchmark(
        [&G_dsmp]() { benchmark_do_not_optimize(ge_check_subgroup_precomp_vartime(G_dsmp)); },
        "ge_check_subgroup_precomp_vartime");

    benchmark(
        [&scalar]()
        {
            ge_p1p1 point;

            ge_scalarmult_base_ct(&point, scalar);
        },
        "ge_scalarmult_base_ct");

    benchmark(
        [&G_point3, &scalar]()
        {
            ge_p1p1 point;

            ge_scalarmult_ct(&point, scalar, &G_point3);
        },
        "ge_scalarmult_ct");

    benchmark(
        [&G_point3, &scalar]()
        {
            ge_p1p1 point;

            ge_double_scalarmult_base_negate_vartime(&point, scalar, &G_point3, scalar);
        },
        "ge_double_scalarmult_base_negate_vartime");

    benchmark(
        [&G_point3, &scalar, &G_dsmp]()
        {
            ge_p1p1 point;

            ge_double_scalarmult_negate_vartime(&point, scalar, &G_point3, scalar, G_dsmp);
        },
        "ge_double_scalarmult_negate_vartime");

    // =========================================================================
    // Section 5b: Batch Scalar Multiplication (opt-in via --batch)
    // =========================================================================

    if (bench_batch)
    {
        std::cout << std::endl << "Batch Scalar Multiplication" << std::endl << std::endl;

        benchmark_header();

        {
            // Setup batch test data
            constexpr size_t BATCH_COUNTS[] = {4, 8, 16, 64};
            for (size_t bi = 0; bi < sizeof(BATCH_COUNTS) / sizeof(BATCH_COUNTS[0]); bi++)
            {
                const size_t N = BATCH_COUNTS[bi];
                std::vector<unsigned char> batch_scalars(N * 32);
                std::vector<unsigned char> batch_b_scalars(N * 32);
                std::vector<ge_p3> batch_points(N);
                std::vector<ge_p2> batch_results(N);

                for (size_t i = 0; i < N; i++)
                {
                    for (int j = 0; j < 32; j++)
                    {
                        batch_scalars[i * 32 + j] = static_cast<unsigned char>(j + i + 1);
                        batch_b_scalars[i * 32 + j] = static_cast<unsigned char>(j + i + 65);
                    }
                    batch_scalars[i * 32 + 0] &= 248;
                    batch_scalars[i * 32 + 31] &= 127;
                    batch_scalars[i * 32 + 31] |= 64;
                    batch_b_scalars[i * 32 + 31] &= 0x7F;
                    ge_p1p1 tmp;
                    ge_scalarmult_base_ct(&tmp, batch_scalars.data() + i * 32);
                    ge_p1p1_to_p3(&batch_points[i], &tmp);
                }

                char label[64];

                std::snprintf(label, sizeof(label), "ge_scalarmult_ct_batch(N=%zu)", N);
                benchmark(
                    [&]()
                    { ge_scalarmult_ct_batch(batch_results.data(), batch_scalars.data(), batch_points.data(), N); },
                    label);

                std::snprintf(label, sizeof(label), "ge_dsm_base_negate_vt_batch(N=%zu)", N);
                benchmark(
                    [&]()
                    {
                        ge_double_scalarmult_base_negate_vartime_batch(
                            batch_results.data(), batch_scalars.data(), batch_points.data(), batch_b_scalars.data(), N);
                    },
                    label);

                // Shared-scalar batch DSM: use first a/b scalar pair for all N point pairs
                std::snprintf(label, sizeof(label), "ge_dsm_negate_vt_batch_ss(N=%zu)", N);
                benchmark(
                    [&]()
                    {
                        ge_double_scalarmult_negate_vartime_batch_ss(
                            batch_results.data(),
                            batch_scalars.data(),
                            batch_points.data(),
                            batch_b_scalars.data(),
                            batch_points.data(),
                            N);
                    },
                    label);

                // Shared-scalar batch DSM returning p3: same as above but outputs ge_p3
                std::vector<ge_p3> batch_p3_results(N);
                std::snprintf(label, sizeof(label), "ge_dsm_negate_vt_batch_ss_p3(N=%zu)", N);
                benchmark(
                    [&]()
                    {
                        ge_double_scalarmult_negate_vartime_batch_ss_p3(
                            batch_p3_results.data(),
                            batch_scalars.data(),
                            batch_points.data(),
                            batch_b_scalars.data(),
                            batch_points.data(),
                            N);
                    },
                    label);
            }
        }
    } // bench_batch

    // =========================================================================
    // Section 5c: Multi-Scalar Multiplication (opt-in via --msm)
    // =========================================================================

    if (bench_msm)
    {
        std::cout << std::endl << "Multi-Scalar Multiplication" << std::endl << std::endl;

        benchmark_header();

        {
            constexpr size_t MSM_COUNTS[] = {2, 4, 8, 16, 32, 64, 128, 256};
            for (size_t mi = 0; mi < sizeof(MSM_COUNTS) / sizeof(MSM_COUNTS[0]); mi++)
            {
                const size_t N = MSM_COUNTS[mi];
                std::vector<unsigned char> msm_scalars(N * 32);
                std::vector<ge_p3> msm_points(N);
                ge_p3 msm_result;

                for (size_t i = 0; i < N; i++)
                {
                    for (int j = 0; j < 32; j++)
                        msm_scalars[i * 32 + j] = static_cast<unsigned char>(j + i + 1);
                    msm_scalars[i * 32 + 0] &= 248;
                    msm_scalars[i * 32 + 31] &= 127;
                    msm_scalars[i * 32 + 31] |= 64;
                    ge_p1p1 tmp;
                    ge_scalarmult_base_ct(&tmp, msm_scalars.data() + i * 32);
                    ge_p1p1_to_p3(&msm_points[i], &tmp);
                }

                char label[80];

                // Benchmark MSM
                std::snprintf(label, sizeof(label), "ge_multiscalar_mul_vartime(N=%zu)", N);
                benchmark(
                    [&]()
                    {
                        ge_multiscalar_mul_vartime(&msm_result, msm_scalars.data(), msm_points.data(), N);
                        benchmark_do_not_optimize(msm_result);
                    },
                    label);

                // Benchmark serial (N individual scalarmults + additions) for comparison
                std::snprintf(label, sizeof(label), "serial_scalarmult+add(N=%zu)", N);
                benchmark(
                    [&]()
                    {
                        ge_p1p1 tmp;
                        ge_p3 acc;
                        ge_scalarmult_ct(&tmp, msm_scalars.data(), &msm_points[0]);
                        ge_p1p1_to_p3(&acc, &tmp);
                        for (size_t k = 1; k < N; k++)
                        {
                            ge_p3 ri;
                            ge_scalarmult_ct(&tmp, msm_scalars.data() + k * 32, &msm_points[k]);
                            ge_p1p1_to_p3(&ri, &tmp);
                            ge_cached cached;
                            ge_p3_to_cached(&cached, &ri);
                            ge_add(&tmp, &acc, &cached);
                            ge_p1p1_to_p3(&acc, &tmp);
                        }
                        benchmark_do_not_optimize(acc);
                    },
                    label);

                // Benchmark MSM with base point
                unsigned char base_scalar[32];
                for (int j = 0; j < 32; j++)
                    base_scalar[j] = static_cast<unsigned char>(j + 129);
                base_scalar[0] &= 248;
                base_scalar[31] &= 127;
                base_scalar[31] |= 64;

                std::snprintf(label, sizeof(label), "ge_msm_base_vartime(N=%zu)", N);
                benchmark(
                    [&]()
                    {
                        ge_multiscalar_mul_base_vartime(
                            &msm_result, msm_scalars.data(), msm_points.data(), N, base_scalar);
                        benchmark_do_not_optimize(msm_result);
                    },
                    label);
            }
        }
    } // bench_msm

    // =========================================================================
    // Section 6: Scalar Operations
    // =========================================================================

    std::cout << std::endl << "Scalar Operations" << std::endl << std::endl;

    benchmark_header();

    benchmark(
        []()
        {
            unsigned char s[32] = {0};
            sc_0(s);
        },
        "sc_0");

    benchmark(
        [&scalar]()
        {
            unsigned char bytes[32] = {0};

            sc_add(bytes, scalar, scalar);
        },
        "sc_add");

    benchmark(
        [&scalar]()
        {
            unsigned char bytes[32] = {0};

            sc_sub(bytes, scalar, scalar);
        },
        "sc_sub");

    benchmark(
        [&scalar]()
        {
            unsigned char bytes[32] = {0};

            sc_mul(bytes, scalar, scalar);
        },
        "sc_mul");

    benchmark(
        [&scalar]()
        {
            unsigned char bytes[32] = {0};

            sc_muladd(bytes, scalar, scalar, scalar);
        },
        "sc_muladd");

    benchmark(
        [&scalar]()
        {
            unsigned char bytes[32] = {0};

            sc_mulsub(bytes, scalar, scalar, scalar);
        },
        "sc_mulsub");

    benchmark([&scalar]() { sc_reduce((unsigned char *)&scalar, 32); }, "sc_reduce(32)");

    benchmark(
        [&scalar]()
        {
            unsigned char buf[64] = {0};
            std::memcpy(buf, scalar, 32);
            sc_reduce(buf, 64);
        },
        "sc_reduce(64)");

    benchmark([&scalar]() { sc_clamp((unsigned char *)&scalar); }, "sc_clamp");

    benchmark(
        [&scalar]() { benchmark_do_not_optimize(sc_check_reduced((unsigned char *)&scalar)); }, "sc_check_reduced");

    benchmark(
        [&scalar]() { benchmark_do_not_optimize(sc_check_clamped((unsigned char *)&scalar)); }, "sc_check_clamped");

    benchmark([&scalar]() { benchmark_do_not_optimize(sc_isnonzero(scalar)); }, "sc_isnonzero");

    // =========================================================================
    // Section 7: Ristretto255 Operations
    // =========================================================================

    std::cout << std::endl << "Ristretto255 Operations" << std::endl << std::endl;

    benchmark_header();

    // Setup: encode the base point as ristretto255
    unsigned char ristretto_encoded[32];
    ristretto255_encode(ristretto_encoded, &G_point3);

    // Setup: a 64-byte uniform input for hash-to-group
    const unsigned char uniform_bytes[64] = {
        0x5d, 0x1b, 0xe0, 0x9e, 0x3d, 0x0c, 0x82, 0xfc, 0x53, 0x81, 0x12, 0x49, 0x0e, 0x35, 0x70, 0x19,
        0x79, 0xd9, 0x9e, 0x06, 0xca, 0x3e, 0x2b, 0x5b, 0x54, 0xbf, 0xfe, 0x8b, 0x4d, 0xc7, 0x72, 0xc1,
        0x4d, 0x98, 0xb6, 0x96, 0xa1, 0xbb, 0xfb, 0x5c, 0xa3, 0x2c, 0x43, 0x6c, 0xc6, 0x1c, 0x16, 0x56,
        0x37, 0x90, 0x30, 0x6c, 0x79, 0xea, 0xca, 0x77, 0x05, 0x66, 0x8b, 0x47, 0xdf, 0xfe, 0x5b, 0xb6};

    benchmark(
        [&ristretto_encoded]()
        {
            ge_p3 point;
            benchmark_do_not_optimize(ristretto255_decode(&point, ristretto_encoded));
        },
        "ristretto255_decode");

    benchmark(
        [&G_point3]()
        {
            unsigned char bytes[32];
            ristretto255_encode(bytes, &G_point3);
        },
        "ristretto255_encode");

    benchmark(
        [&G_point3, &H_point3]() { benchmark_do_not_optimize(ristretto255_equals(&G_point3, &H_point3)); },
        "ristretto255_equals");

    benchmark(
        [&uniform_bytes]()
        {
            ge_p3 point;
            ristretto255_from_uniform_bytes(&point, uniform_bytes);
        },
        "ristretto255_from_uniform_bytes");

    benchmark_teardown(bench_state);
}
