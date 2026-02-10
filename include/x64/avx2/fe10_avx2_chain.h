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
#ifndef ED25519_X64_AVX2_FE10_AVX2_CHAIN_H
#define ED25519_X64_AVX2_FE10_AVX2_CHAIN_H

/**
 * @file fe10_avx2_chain.h
 * @brief Chain operation macros for AVX2 radix-2^25.5 field arithmetic.
 *
 * These macros are the AVX2 analogues of fe51_chain.h and fe_ifma_chain.h.
 * They map the generic chain names (fe10_avx2_chain_mul, etc.) to the
 * force-inlined fe10 implementations in fe10_avx2.h.
 *
 * Used by the MSVC AVX2 scalarmult TUs where the entire scalarmult loop
 * runs in radix-2^25.5 representation. Since fe10 arithmetic uses plain
 * int64_t (no 128-bit types), these are safe to force-inline on MSVC
 * without the register spilling problems that plague fe51 chain ops.
 */

#include "x64/avx2/fe10_avx2.h"

#define fe10_avx2_chain_mul fe10_mul_avx2
#define fe10_avx2_chain_sq fe10_sq_avx2
#define fe10_avx2_chain_sq2 fe10_sq2_avx2
#define fe10_avx2_chain_sqn fe10_sqn_avx2

#endif // ED25519_X64_AVX2_FE10_AVX2_CHAIN_H
