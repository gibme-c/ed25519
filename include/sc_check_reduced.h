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
 * @file sc_check_reduced.h
 * @brief Validate that a scalar is reduced modulo l.
 *
 * Checks that a 32-byte scalar is strictly less than l. This is an input
 * validation step: signature components must be in [0, l-1] to prevent
 * malleability attacks where an attacker could modify a valid signature by
 * adding multiples of l.
 */

#ifndef ED25519_SC_CHECK_REDUCED_H
#define ED25519_SC_CHECK_REDUCED_H

#include "ed25519_platform.h"

/**
 * @brief Checks if a 32-byte scalar is in the valid range [0, l).
 *
 * @param s Input 32-byte scalar.
 * @return 0 if valid (s < l), non-zero otherwise.
 */
#if ED25519_PLATFORM_64BIT
int sc_check_reduced_x64(const unsigned char *s);
static inline int sc_check_reduced(const unsigned char *s)
{
    return sc_check_reduced_x64(s);
}
#else
int sc_check_reduced_portable(const unsigned char *s);
static inline int sc_check_reduced(const unsigned char *s)
{
    return sc_check_reduced_portable(s);
}
#endif

#endif // ED25519_SC_CHECK_REDUCED_H
