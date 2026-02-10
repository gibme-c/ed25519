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
 * @file sc_reduce.h
 * @brief Reduce a scalar modulo the group order l.
 *
 * Takes either 32 or 64 bytes and reduces them modulo l. The 64-byte
 * variant is used after hashing in Ed25519 signing and verification: SHA-512
 * produces 64 bytes, which must be reduced to a scalar modulo l before use
 * in the signature equation. The 32-byte variant handles already-small
 * values that just need a single conditional subtraction.
 */

#ifndef ED25519_SC_REDUCE_H
#define ED25519_SC_REDUCE_H

#include "ed25519_platform.h"

#include <cstddef>

/**
 * @brief Reduces a scalar modulo l in place.
 *
 * Supports both 32-byte and 64-byte inputs. The result is stored in the first 32 bytes of s.
 *
 * @param s Input/output scalar buffer.
 * @param len Length of the input: 32 or 64 bytes.
 */
void sc_reduce_portable(unsigned char *s, size_t len);

#if ED25519_PLATFORM_64BIT
void sc_reduce_x64(unsigned char *s, size_t len);
static inline void sc_reduce(unsigned char *s, size_t len)
{
    sc_reduce_x64(s, len);
}
#else
static inline void sc_reduce(unsigned char *s, size_t len)
{
    sc_reduce_portable(s, len);
}
#endif

#endif // ED25519_SC_REDUCE_H
