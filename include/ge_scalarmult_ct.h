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
 * @file ge_scalarmult_ct.h
 * @brief Constant-time variable-base scalar multiplication.
 *
 * Computes r = s * A where s is a scalar and A is any curve point. This is
 * the core operation for Diffie-Hellman key exchange (multiply your private
 * scalar by the other party's public point). "Constant-time" means the
 * execution path doesn't depend on the scalar value, which is critical
 * because the scalar is typically a secret key. The implementation uses a
 * windowed method with constant-time table lookups via ge_cached_cmov to
 * avoid leaking scalar bits through timing or cache side channels.
 */

#ifndef ED25519_GE_SCALARMULT_CT_H
#define ED25519_GE_SCALARMULT_CT_H

#include "ge.h"

#if ED25519_SIMD
#include "ed25519_dispatch.h"
#endif

/**
 * @brief Computes r = a * A in constant time.
 *
 * @pre a[31] must be <= 127. This is satisfied by any scalar that has been
 *      clamped via sc_clamp() (which clears bit 7 of byte 31) or reduced
 *      modulo l via sc_reduce() (l < 2^253, so all reduced scalars have
 *      byte 31 < 0x20). Raw 32-byte scalars with byte 31 >= 128 produce
 *      silently wrong output.
 *
 * @param r Output extended point.
 * @param a Input 32-byte scalar.
 * @param A Input extended point (base point).
 */
#if ED25519_PLATFORM_64BIT
void ge_scalarmult_x64_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A);
static inline void ge_scalarmult_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A)
{
#if ED25519_SIMD
    ed25519_get_dispatch().scalarmult_ct(t, a, A);
#else
    ge_scalarmult_x64_ct(t, a, A);
#endif
}
#else
void ge_scalarmult_portable_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A);
static inline void ge_scalarmult_ct(ge_p1p1 *t, const unsigned char *a, const ge_p3 *A)
{
    ge_scalarmult_portable_ct(t, a, A);
}
#endif

#endif // ED25519_GE_SCALARMULT_CT_H
