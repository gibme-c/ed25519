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
 * @file cpuid.cpp
 * @brief Runtime CPU feature detection via CPUID and XGETBV.
 *
 * Detection sequence:
 *   1. CPUID(1).ECX bit 27  -> OSXSAVE (OS supports XGETBV)
 *   2. XGETBV(0) bits 1:2   -> OS enabled XMM + YMM state saving
 *   3. CPUID(7,0).EBX bit 5 -> AVX2
 *   4. XGETBV(0) bits 5:7   -> OS enabled OPMASK + ZMM_Hi256 + Hi16_ZMM (for AVX-512)
 *   5. CPUID(7,0).EBX bit 16 -> AVX-512F
 *   6. CPUID(7,0).EBX bit 21 -> AVX-512 IFMA
 */

#include "ed25519_cpuid.h"

#if ED25519_PLATFORM_X64

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

// XGETBV via inline assembly for GCC/Clang (the intrinsic requires -mxsave target)
#if !defined(_MSC_VER)
static inline uint64_t ed25519_xgetbv(uint32_t index)
{
    uint32_t eax, edx;
    __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
    return (static_cast<uint64_t>(edx) << 32) | eax;
}
#endif

static uint32_t ed25519_detect_cpu_features()
{
    uint32_t flags = 0;

#if defined(_MSC_VER)
    int regs[4]; // EAX, EBX, ECX, EDX

    // CPUID leaf 1: check OSXSAVE (ECX bit 27)
    __cpuid(regs, 1);
    const bool osxsave = (regs[2] & (1 << 27)) != 0;

    if (!osxsave)
    {
        return 0;
    }

    // XGETBV(0): check OS has enabled XMM (bit 1) and YMM (bit 2) state saving
    const uint64_t xcr0 = _xgetbv(0);
    const bool ymm_enabled = (xcr0 & 0x06) == 0x06; // bits 1 and 2

    if (!ymm_enabled)
    {
        return 0;
    }

    // CPUID leaf 7, subleaf 0: extended feature flags in EBX
    __cpuidex(regs, 7, 0);
    const uint32_t ebx7 = static_cast<uint32_t>(regs[1]);

    // AVX2: EBX bit 5
    if (ebx7 & (1 << 5))
    {
        flags |= ED25519_CPU_AVX2;
    }

    // AVX-512 requires OS support for OPMASK (bit 5), ZMM_Hi256 (bit 6), Hi16_ZMM (bit 7)
    const bool zmm_enabled = (xcr0 & 0xE0) == 0xE0; // bits 5, 6, 7

    if (zmm_enabled)
    {
        // AVX-512F: EBX bit 16
        if (ebx7 & (1 << 16))
        {
            flags |= ED25519_CPU_AVX512F;
        }

        // AVX-512 IFMA: EBX bit 21
        if (ebx7 & (1 << 21))
        {
            flags |= ED25519_CPU_AVX512IFMA;
        }
    }

#else // GCC / Clang
    unsigned int eax, ebx, ecx, edx;

    // CPUID leaf 1: check OSXSAVE (ECX bit 27)
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
    {
        return 0;
    }

    const bool osxsave = (ecx & (1 << 27)) != 0;

    if (!osxsave)
    {
        return 0;
    }

    // XGETBV(0): check OS has enabled XMM (bit 1) and YMM (bit 2) state saving
    const uint64_t xcr0 = ed25519_xgetbv(0);
    const bool ymm_enabled = (xcr0 & 0x06) == 0x06;

    if (!ymm_enabled)
    {
        return 0;
    }

    // CPUID leaf 7, subleaf 0: extended feature flags in EBX
    if (!__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
    {
        return 0;
    }

    // AVX2: EBX bit 5
    if (ebx & (1 << 5))
    {
        flags |= ED25519_CPU_AVX2;
    }

    // AVX-512 requires OS support for OPMASK (bit 5), ZMM_Hi256 (bit 6), Hi16_ZMM (bit 7)
    const bool zmm_enabled = (xcr0 & 0xE0) == 0xE0;

    if (zmm_enabled)
    {
        // AVX-512F: EBX bit 16
        if (ebx & (1 << 16))
        {
            flags |= ED25519_CPU_AVX512F;
        }

        // AVX-512 IFMA: EBX bit 21
        if (ebx & (1 << 21))
        {
            flags |= ED25519_CPU_AVX512IFMA;
        }
    }

#endif

    return flags;
}

uint32_t ed25519_cpu_features()
{
    static const uint32_t cached = ed25519_detect_cpu_features();

    return cached;
}

#endif // ED25519_PLATFORM_X64
