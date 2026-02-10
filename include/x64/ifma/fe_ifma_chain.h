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
#ifndef ED25519_X64_IFMA_FE_IFMA_CHAIN_H
#define ED25519_X64_IFMA_FE_IFMA_CHAIN_H

/**
 * @file fe_ifma_chain.h
 * @brief Chain operation macros for AVX-512 IFMA field arithmetic.
 *
 * These macros are the IFMA analogues of the fe51_chain_* macros in
 * x64/fe51_chain.h. They map the generic chain names to force-inlined
 * IFMA implementations, which are used inside the inline ge operations
 * in each IFMA scalarmult TU (ge_add_ifma, ge_sub_ifma, etc.).
 *
 * IFMA intrinsics work on both MSVC and GCC/Clang without the uint128
 * struct emulation problem, so there's no compiler split here -- both
 * compilers use the same force-inlined functions.
 *
 * Two sets of macros are provided:
 *   - Normalizing: fe_ifma_chain_mul, fe_ifma_chain_sq, etc. Safe for
 *     any input (carry-propagates both operands before IFMA rounds).
 *   - Non-normalizing (_nn/_n suffixes): skip input carry propagation
 *     when limb bit-width bounds are known safe. Used in the optimized
 *     ge bodies where fe_normalize_weak is placed at the few problematic
 *     outputs instead of normalizing every input.
 */

#include "x64/ifma/fe_ifma.h"

/* Normalizing variants (original, safe for any input) */
#define fe_ifma_chain_mul fe_mul_ifma
#define fe_ifma_chain_sq fe_sq_ifma
#define fe_ifma_chain_sq2 fe_sq2_ifma
#define fe_ifma_chain_sqn fe_sqn_ifma

/* No-normalization variants: inputs must have limbs ≤52 bits.
 * Use these when all IFMA inputs are known to be within bounds
 * (IFMA mul/sq outputs ≤51 bits, fe_add of two ≤51-bit ≤52, fe_sub ≤52). */
#define fe_ifma_chain_mul_nn fe_mul_ifma_nn
#define fe_ifma_chain_sq_n fe_sq_ifma_n
#define fe_ifma_chain_sq2_n fe_sq2_ifma_n
#define fe_ifma_chain_sqn_n fe_sqn_ifma_n

#endif // ED25519_X64_IFMA_FE_IFMA_CHAIN_H
