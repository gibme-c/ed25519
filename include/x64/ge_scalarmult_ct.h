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
 * @file x64_ct.h
 * @brief x64 constant-time scalar multiplication implementation.
 */

#ifndef ED25519_X64_SCALARMULT_CT_H
#define ED25519_X64_SCALARMULT_CT_H

#include "ge.h"

/**
 * @brief x64 implementation of constant-time scalar multiplication: r = a * A.
 *
 * @param r Output extended point.
 * @param a Input 32-byte scalar.
 * @param A Input extended point.
 */
void ge_scalarmult_x64_ct(ge_p1p1 *r, const unsigned char *a, const ge_p3 *A);

#endif // ED25519_X64_SCALARMULT_CT_H
