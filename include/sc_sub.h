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
 * @file sc_sub.h
 * @brief Scalar subtraction modulo the group order l.
 *
 * Subtracts one 32-byte scalar from another modulo l. If the subtraction
 * underflows, the result wraps around by adding l (so you always get a value
 * in [0, l-1]). Same platform split as sc_add.
 */

#ifndef ED25519_SC_SUB_H
#define ED25519_SC_SUB_H

#include "ed25519_platform.h"

/**
 * @brief Subtracts two scalars modulo l: s = a - b mod l.
 *
 * @param s Output 32-byte scalar.
 * @param a First input 32-byte scalar.
 * @param b Second input 32-byte scalar.
 */
#if ED25519_PLATFORM_64BIT
void sc_sub_x64(unsigned char *s, const unsigned char *a, const unsigned char *b);
static inline void sc_sub(unsigned char *s, const unsigned char *a, const unsigned char *b)
{
    sc_sub_x64(s, a, b);
}
#else
void sc_sub_portable(unsigned char *s, const unsigned char *a, const unsigned char *b);
static inline void sc_sub(unsigned char *s, const unsigned char *a, const unsigned char *b)
{
    sc_sub_portable(s, a, b);
}
#endif

#endif // ED25519_SC_SUB_H
