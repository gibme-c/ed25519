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
 * @file sc_isnonzero.h
 * @brief Check if a scalar is nonzero.
 *
 * ORs all 32 bytes together and checks if the result is nonzero.
 * Constant-time: always reads every byte regardless of the value.
 */

#ifndef ED25519_SC_ISNONZERO_H
#define ED25519_SC_ISNONZERO_H

#include "ed25519_platform.h"

#include <cstdint>
#include <cstring>

/**
 * @brief Checks if a 32-byte scalar is nonzero.
 *
 * @param s Input 32-byte scalar.
 * @return Nonzero value if s != 0, zero if s == 0.
 */
static inline int sc_isnonzero(const unsigned char *s)
{
#if ED25519_PLATFORM_64BIT
    uint64_t v[4];
    std::memcpy(v, s, 32);
    uint64_t d = v[0] | v[1] | v[2] | v[3];
    return -(int)((d | (~d + 1)) >> 63);
#else
    unsigned int d = 0;
    for (int i = 0; i < 32; i++)
    {
        d |= s[i];
    }
    return (1 & ((d - 1) >> 8)) - 1;
#endif
}

#endif // ED25519_SC_ISNONZERO_H
