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
 * @file sc_mulsub.h
 * @brief Scalar multiply-subtract modulo the group order l.
 *
 * Computes s = c - a*b mod l. The mirror image of sc_muladd, used in
 * certain verification and proof schemes that express the signature equation
 * with a subtraction instead of an addition.
 */

#ifndef ED25519_SC_MULSUB_H
#define ED25519_SC_MULSUB_H

#include "ed25519_platform.h"

/**
 * @brief Computes s = (c - a * b) mod l.
 *
 * @param s Output 32-byte scalar.
 * @param a First multiplicand (32-byte scalar).
 * @param b Second multiplicand (32-byte scalar).
 * @param c Value to subtract from (32-byte scalar).
 */
#if ED25519_PLATFORM_64BIT
void sc_mulsub_x64(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c);
static inline void sc_mulsub(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c)
{
    sc_mulsub_x64(s, a, b, c);
}
#else
void sc_mulsub_portable(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c);
static inline void sc_mulsub(unsigned char *s, const unsigned char *a, const unsigned char *b, const unsigned char *c)
{
    sc_mulsub_portable(s, a, b, c);
}
#endif

#endif // ED25519_SC_MULSUB_H
