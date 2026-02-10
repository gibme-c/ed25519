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
 * @file ristretto255_equals.h
 * @brief Constant-time ristretto255 equivalence check.
 *
 * Tests whether two ge_p3 points represent the same ristretto255 element.
 * Two points are ristretto-equivalent if they differ only by cofactor or
 * sign -- this check catches all such cases via a cross-ratio test
 * (X1*Y2 == Y1*X2 or Y1*Y2 == X1*X2) without needing to encode first.
 * Useful when you want to compare points in their internal representation
 * without paying for two full encodings.
 *
 * Fully constant-time.
 */

#ifndef ED25519_RISTRETTO255_EQUALS_H
#define ED25519_RISTRETTO255_EQUALS_H

#include "ge.h"

/**
 * @brief Checks ristretto255 equivalence of two ge_p3 points.
 *
 * @param p First extended point.
 * @param q Second extended point.
 * @return 1 if p and q represent the same ristretto255 element, 0 otherwise.
 */
int ristretto255_equals(const ge_p3 *p, const ge_p3 *q);

#endif // ED25519_RISTRETTO255_EQUALS_H
