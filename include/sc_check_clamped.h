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
 * @file sc_check_clamped.h
 * @brief Validate that a scalar is properly clamped per RFC 8032.
 *
 * Verifies that the three clamping conditions hold: bits 0-2 are clear,
 * bit 255 is clear, and bit 254 is set. This is a check (not a
 * transformation) -- use sc_clamp to apply clamping, and this to verify
 * that someone else's scalar was properly clamped.
 */

#ifndef ED25519_SC_CHECK_CLAMPED_H
#define ED25519_SC_CHECK_CLAMPED_H

#include "sc_clamp.h"

#include <cstring>

/**
 * @brief Checks if a 32-byte scalar is properly clamped per RFC 8032.
 *
 * @param s Input 32-byte scalar.
 * @return 0 if properly clamped, non-zero otherwise.
 */
static inline int sc_check_clamped(const unsigned char *s)
{
    unsigned char check[32];
    std::memmove(check, s, sizeof(check));
    sc_clamp(check);

    /* Constant-time comparison: accumulate XOR of all bytes */
    unsigned int diff = 0;
    for (int i = 0; i < 32; i++)
        diff |= check[i] ^ s[i];
    return diff != 0;
}

#endif // ED25519_SC_CHECK_CLAMPED_H
