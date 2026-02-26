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
 * @file fe_mul121666.h
 * @brief Multiply a field element by the constant 121666.
 *
 * This is the Montgomery curve constant (A+2)/4 = 121666 used in the X25519
 * Diffie-Hellman function (RFC 7748). Much cheaper than a general fe_mul since
 * one operand is a small known constant -- just 5 (or 10) scalar multiplies
 * plus carry propagation, vs the full schoolbook product.
 */

#ifndef ED25519_FE_MUL121666_H
#define ED25519_FE_MUL121666_H

#include "fe.h"

/**
 * @brief Multiplies a field element by 121666: h = 121666 * f.
 *
 * Can overlap h with f.
 *
 * @param h Output field element.
 * @param f Input field element.
 */
#if ED25519_PLATFORM_64BIT
void fe_mul121666_x64(fe h, const fe f);
static inline void fe_mul121666(fe h, const fe f)
{
    fe_mul121666_x64(h, f);
}
#else
void fe_mul121666_portable(fe h, const fe f);
static inline void fe_mul121666(fe h, const fe f)
{
    fe_mul121666_portable(h, f);
}
#endif

#endif // ED25519_FE_MUL121666_H
