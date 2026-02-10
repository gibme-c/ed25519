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
 * @file ct_barrier.h
 * @brief Compiler optimization barriers for constant-time code.
 *
 * Prevents the compiler from reasoning about the value of a variable,
 * stopping it from converting branchless arithmetic (XOR-mask, subtract-
 * and-shift) into conditional branches. Critical for constant-time crypto
 * where the compiler must not introduce data-dependent control flow.
 *
 * GCC/Clang: inline asm constraint marks the value as read+written,
 * forcing the compiler to treat it as opaque.
 * MSVC: volatile round-trip through memory.
 */

#ifndef ED25519_CT_BARRIER_H
#define ED25519_CT_BARRIER_H

#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)

static inline uint32_t ct_barrier_u32(uint32_t x)
{
    __asm__ __volatile__("" : "+r"(x));
    return x;
}

static inline uint64_t ct_barrier_u64(uint64_t x)
{
    __asm__ __volatile__("" : "+r"(x));
    return x;
}

#else

static inline uint32_t ct_barrier_u32(uint32_t x)
{
    volatile uint32_t v = x;
    return v;
}

static inline uint64_t ct_barrier_u64(uint64_t x)
{
    volatile uint64_t v = x;
    return v;
}

#endif

#endif // ED25519_CT_BARRIER_H
