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
 * @file ge_check_subgroup_precomp_vartime.h
 * @brief Subgroup membership check using precomputation (non-negated input).
 *
 * Verifies that l*A = 0 (the identity point), which proves that A is in the
 * prime-order subgroup of the Ed25519 curve. The full curve has order 8*l,
 * so not every point on the curve is in the subgroup -- this check catches
 * "small subgroup" points that could be used in attacks on poorly designed
 * protocols. Uses the precomputed dsm table for A to speed up the scalar
 * multiplication by l.
 *
 * @note ge_frombytes_vartime() validates on-curve but does NOT check
 * subgroup membership. Callers should use this function in the following
 * scenarios:
 *
 * - **Mandatory**: Diffie-Hellman / X25519-like key exchange (prevents
 *   small-subgroup attacks that leak private key bits).
 * - **Recommended**: Signature verification (prevents cofactor-related
 *   edge cases; some Ed25519 interpretations require it).
 * - **Optional**: When the protocol already clears the cofactor (e.g.,
 *   clamped scalars with cofactored verification).
 */

#ifndef ED25519_GE_CHECK_SUBGROUP_PRECOMP_VARTIME_H
#define ED25519_GE_CHECK_SUBGROUP_PRECOMP_VARTIME_H

#include "ge.h"

/**
 * @brief Checks if a point is in the prime-order subgroup (variable-time).
 *
 * Verifies that l * A = 0 (the neutral element) using precomputed tables.
 * Unlike ge_check_subgroup_precomp_negate_vartime, this variant expects
 * a precomputation table built from a non-negated point (decoded via
 * ge_frombytes_vartime).
 *
 * @param A Precomputation table for the point to check.
 * @return 0 if the point is in the subgroup, nonzero otherwise.
 */
#if ED25519_PLATFORM_64BIT
int ge_check_subgroup_precomp_vartime_x64(const ge_dsmp p);
static inline int ge_check_subgroup_precomp_vartime(const ge_dsmp p)
{
    return ge_check_subgroup_precomp_vartime_x64(p);
}
#else
int ge_check_subgroup_precomp_vartime_portable(const ge_dsmp p);
static inline int ge_check_subgroup_precomp_vartime(const ge_dsmp p)
{
    return ge_check_subgroup_precomp_vartime_portable(p);
}
#endif

#endif // ED25519_GE_CHECK_SUBGROUP_PRECOMP_VARTIME_H
