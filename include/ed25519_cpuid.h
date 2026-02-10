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
 * @file ed25519_cpuid.h
 * @brief Runtime CPU feature detection for SIMD dispatch.
 *
 * Detects AVX2, AVX-512F, and AVX-512 IFMA support at runtime via CPUID and
 * XGETBV. Results are cached on first call — no overhead after initialization.
 * On non-x86_64 platforms, all feature queries return false.
 *
 * These functions are used by the scalarmult dispatch headers to select the
 * best available implementation at runtime when SIMD support is compiled in.
 */

#ifndef ED25519_CPUID_H
#define ED25519_CPUID_H

#include "ed25519_platform.h"

#include <cstdint>

/**
 * @brief Bitmask of detected CPU features relevant to this library.
 */
enum ed25519_cpu_flag : uint32_t
{
    ED25519_CPU_AVX2 = 1 << 0,
    ED25519_CPU_AVX512F = 1 << 1,
    ED25519_CPU_AVX512IFMA = 1 << 2,
};

#if ED25519_PLATFORM_X64

/**
 * @brief Detects CPU features and returns a bitmask of ed25519_cpu_flag values.
 *
 * The result is computed once (via CPUID + XGETBV) and cached in a static local.
 * Subsequent calls return the cached value directly.
 */
uint32_t ed25519_cpu_features();

/**
 * @brief Returns true if the CPU and OS support AVX2.
 */
static inline bool ed25519_has_avx2()
{
    return (ed25519_cpu_features() & ED25519_CPU_AVX2) != 0;
}

/**
 * @brief Returns true if the CPU and OS support AVX-512F.
 */
static inline bool ed25519_has_avx512f()
{
    return (ed25519_cpu_features() & ED25519_CPU_AVX512F) != 0;
}

/**
 * @brief Returns true if the CPU and OS support AVX-512 IFMA.
 */
static inline bool ed25519_has_avx512ifma()
{
    return (ed25519_cpu_features() & ED25519_CPU_AVX512IFMA) != 0;
}

#else

static inline uint32_t ed25519_cpu_features()
{
    return 0;
}

static inline bool ed25519_has_avx2()
{
    return false;
}

static inline bool ed25519_has_avx512f()
{
    return false;
}

static inline bool ed25519_has_avx512ifma()
{
    return false;
}

#endif // ED25519_PLATFORM_X64

#endif // ED25519_CPUID_H
