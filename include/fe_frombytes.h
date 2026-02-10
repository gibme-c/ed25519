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
 * @file fe_frombytes.h
 * @brief Deserialize a field element from bytes.
 *
 * Converts 32 bytes (little-endian, as specified by RFC 8032) into the
 * internal limb representation. The top bit of the last byte is masked off
 * since it's used for the sign of x in the point encoding, not as part of
 * the field element value.
 */

#ifndef ED25519_FE_FROMBYTES_H
#define ED25519_FE_FROMBYTES_H

#include "fe.h"

/**
 * @brief Deserializes 32 bytes (little-endian) into a field element.
 *
 * The resulting field element is reduced but may not be fully canonical.
 *
 * @param h Output field element.
 * @param s Input byte array (32 bytes).
 */
#if ED25519_PLATFORM_64BIT
void fe_frombytes_x64(fe h, const unsigned char *s);
static inline void fe_frombytes(fe h, const unsigned char *s)
{
    fe_frombytes_x64(h, s);
}
#else
void fe_frombytes_portable(fe h, const unsigned char *s);
static inline void fe_frombytes(fe h, const unsigned char *s)
{
    fe_frombytes_portable(h, s);
}
#endif

#endif // ED25519_FE_FROMBYTES_H
