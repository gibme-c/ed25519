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
 * @file slide.h
 * @brief Signed-radix-16 sliding window decomposition of a scalar.
 *
 * Converts a 256-bit scalar into a signed digit representation where each
 * nonzero digit is odd and in the range [-15, 15]. The "sliding window"
 * technique groups consecutive bits into windows, converting them to signed
 * digits and absorbing carries upward. The key insight: since all nonzero
 * digits are odd, you only need to precompute odd multiples of the base
 * point (1, 3, 5, ..., 15), halving the table size. Zeros between windows
 * are processed as free doublings. This is used by the variable-time double
 * scalar multiplication for signature verification.
 */

#ifndef ED25519_SLIDE_H
#define ED25519_SLIDE_H

/**
 * @brief Computes the signed sliding window representation of a scalar.
 *
 * Produces a 256-element array of signed digits in [-15, 15] (odd values only)
 * for use in variable-time scalar multiplication.
 *
 * @param r Output array of 256 signed digits.
 * @param a Input 32-byte scalar.
 */
void slide(signed char *r, const unsigned char *a);

#endif // ED25519_SLIDE_H
