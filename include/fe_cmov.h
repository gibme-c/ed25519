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
 * @file fe_cmov.h
 * @brief Constant-time conditional move for field elements.
 *
 * Replaces f with g when b=1, leaves f unchanged when b=0, and does it
 * without any branches. The trick: compute a bitmask from b (all-ones or
 * all-zeros), then XOR-blend each limb. This prevents the CPU's branch
 * predictor from leaking which path was taken -- critical for operations
 * like scalar multiplication where the branch would reveal secret key bits.
 */

#ifndef ED25519_FE_CMOV_H
#define ED25519_FE_CMOV_H

#include "ed25519_ct_barrier.h"
#include "fe.h"

/**
 * @brief Conditionally replaces f with g in constant time.
 *
 * If b is nonzero, sets f = g. If b is zero, f is unchanged.
 * Runs in constant time to prevent side-channel leakage.
 *
 * @param f Field element to conditionally overwrite.
 * @param g Source field element.
 * @param b Condition flag (0 or 1).
 */
#if ED25519_PLATFORM_64BIT
static inline void fe_cmov(fe f, const fe g, unsigned int b)
{
    uint64_t mask = 0 - (uint64_t)ed25519_ct_barrier_u32(b);
    f[0] ^= mask & (f[0] ^ g[0]);
    f[1] ^= mask & (f[1] ^ g[1]);
    f[2] ^= mask & (f[2] ^ g[2]);
    f[3] ^= mask & (f[3] ^ g[3]);
    f[4] ^= mask & (f[4] ^ g[4]);
}
#else
static inline void fe_cmov(fe f, const fe g, unsigned int b)
{
    int32_t mask = -(int32_t)ed25519_ct_barrier_u32(b);
    f[0] ^= mask & (f[0] ^ g[0]);
    f[1] ^= mask & (f[1] ^ g[1]);
    f[2] ^= mask & (f[2] ^ g[2]);
    f[3] ^= mask & (f[3] ^ g[3]);
    f[4] ^= mask & (f[4] ^ g[4]);
    f[5] ^= mask & (f[5] ^ g[5]);
    f[6] ^= mask & (f[6] ^ g[6]);
    f[7] ^= mask & (f[7] ^ g[7]);
    f[8] ^= mask & (f[8] ^ g[8]);
    f[9] ^= mask & (f[9] ^ g[9]);
}
#endif

#endif // ED25519_FE_CMOV_H
