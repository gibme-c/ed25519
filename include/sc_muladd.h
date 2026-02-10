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
 * @file sc_muladd.h
 * @brief Scalar multiply-add modulo the group order l.
 *
 * Computes s = a*b + c mod l in one shot. This is the core of Ed25519
 * signing: the signature component s = r + H(R,A,M)*a mod l, where r is the
 * nonce scalar, a is the private key, and H(R,A,M) is the hash challenge.
 * Fusing the multiply and add avoids a separate reduction step on the
 * intermediate product.
 */

#ifndef ED25519_SC_MULADD_H
#define ED25519_SC_MULADD_H

#include "ed25519_platform.h"

/**
 * @brief Computes s = (a * b + c) mod l.
 *
 * @param s Output 32-byte scalar.
 * @param a First multiplicand (32-byte scalar).
 * @param b Second multiplicand (32-byte scalar).
 * @param c Addend (32-byte scalar).
 */
#if ED25519_PLATFORM_64BIT
void sc_muladd_x64(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c);
static inline void sc_muladd(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c)
{
    sc_muladd_x64(s, a, b, c);
}
#else
void sc_muladd_portable(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c);
static inline void sc_muladd(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c)
{
    sc_muladd_portable(s, a, b, c);
}
#endif

#endif // ED25519_SC_MULADD_H
