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
 * @file equal.h
 * @brief Constant-time byte equality comparison.
 *
 * Returns 1 if two bytes are equal, 0 otherwise, without using any branches.
 * Uses XOR and bit manipulation to produce the result. This is a low-level
 * helper for constant-time table lookups in scalar multiplication.
 */

#ifndef ED25519_EQUAL_H
#define ED25519_EQUAL_H

#include "ct_barrier.h"

#include <cstdint>

/**
 * @brief Tests two bytes for equality in constant time.
 *
 * @param b First byte value (as unsigned char).
 * @param c Second byte value (as unsigned char).
 * @return 1 if equal, 0 otherwise.
 */
static inline unsigned char equal(signed char b, signed char c)
{
    unsigned char ub = b;
    unsigned char uc = c;
    unsigned char x = ub ^ uc; /* 0: yes; 1..255: no */
    uint32_t y = ct_barrier_u32(x); /* 0: yes; 1..255: no */
    y -= 1; /* 4294967295: yes; 0..254: no */
    y >>= 31; /* 1: yes; 0: no */
    return (unsigned char)y;
}

#endif // ED25519_EQUAL_H
