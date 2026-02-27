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
 * @file negative.h
 * @brief Constant-time sign check for signed integers.
 *
 * Returns 1 if the input is negative, 0 otherwise. Extracts the sign bit
 * via an arithmetic right shift, avoiding branches. Used to determine the
 * sign of scalar digits during windowed scalar multiplication.
 */

#ifndef ED25519_NEGATIVE_H
#define ED25519_NEGATIVE_H

#include "ed25519_ct_barrier.h"

/**
 * @brief Returns 1 if the input is negative, 0 otherwise (constant-time).
 *
 * @param b Signed byte value.
 * @return 1 if b < 0, 0 otherwise.
 */
static inline unsigned char negative(signed char b)
{
    unsigned long long x = b; /* 18446744073709551361..18446744073709551615: yes; 0..255: no */
    x = ed25519_ct_barrier_u64(x);
    x >>= 63; /* 1: yes; 0: no */
    return (unsigned char)x;
}

#endif // ED25519_NEGATIVE_H
