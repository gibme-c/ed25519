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
 * @file fe_tobytes.h
 * @brief Serialize a field element to bytes.
 *
 * Converts a field element to 32 bytes in little-endian order. This first
 * performs a full canonical reduction modulo p (ensuring the output is in
 * [0, p-1]) since the internal limb representation can hold values slightly
 * larger than p. The canonical form is essential for consistent encodings
 * and comparisons.
 */

#ifndef ED25519_FE_TOBYTES_H
#define ED25519_FE_TOBYTES_H

#include "fe.h"

/**
 * @brief Serializes a field element to 32 bytes in little-endian order.
 *
 * Fully reduces the field element modulo 2^255 - 19 before serializing.
 *
 * @param s Output byte array (32 bytes).
 * @param h Input field element.
 */
#if ED25519_PLATFORM_64BIT
void fe_tobytes_x64(unsigned char *s, const fe h);
static inline void fe_tobytes(unsigned char *s, const fe h)
{
    fe_tobytes_x64(s, h);
}
#else
void fe_tobytes_portable(unsigned char *s, const fe h);
static inline void fe_tobytes(unsigned char *s, const fe h)
{
    fe_tobytes_portable(s, h);
}
#endif

#endif // ED25519_FE_TOBYTES_H
