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
 * @file ge_scalarmult_base_ct.h
 * @brief Constant-time fixed-base scalar multiplication.
 *
 * Computes r = s * B where B is the Ed25519 base point. This is how you
 * derive a public key from a private scalar, and it's also used to compute
 * the nonce commitment R during signing. Because B is fixed and known at
 * compile time, we can use a large precomputed table of multiples of B
 * (ge_base) to speed things up significantly: instead of ~253 doublings and
 * additions, this processes 4 bits at a time using the table, cutting the
 * doublings to ~63 and using cheaper mixed additions (ge_madd/ge_msub) with
 * the affine table entries.
 */

#ifndef ED25519_GE_SCALARMULT_BASE_CT_H
#define ED25519_GE_SCALARMULT_BASE_CT_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Computes h = a * B where B is the Ed25519 base point (constant-time).
 *
 * Uses a precomputed table of base point multiples.
 *
 * @param h Output extended point.
 * @param a Input 32-byte scalar.
 */
#if ED25519_PLATFORM_64BIT
void ge_scalarmult_base_ct_x64(ge_p1p1 *r, const unsigned char *a);
static inline void ge_scalarmult_base_ct(ge_p1p1 *r, const unsigned char *a)
{
#if ED25519_SIMD
    ed25519_get_dispatch().scalarmult_base_ct(r, a);
#else
    ge_scalarmult_base_ct_x64(r, a);
#endif
}
#else
void ge_scalarmult_base_ct_portable(ge_p1p1 *r, const unsigned char *a);
static inline void ge_scalarmult_base_ct(ge_p1p1 *r, const unsigned char *a)
{
    ge_scalarmult_base_ct_portable(r, a);
}
#endif

#endif // ED25519_GE_SCALARMULT_BASE_CT_H
