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
 * @file fe_isnegative.h
 * @brief Check if a field element is negative.
 *
 * In GF(p) there's no natural notion of "positive" or "negative," so
 * Ed25519 defines a convention: a field element is "negative" if its
 * canonical byte encoding is odd (least significant bit = 1). This is used
 * in point serialization to encode the sign of the x coordinate.
 */

#ifndef ED25519_FE_ISNEGATIVE_H
#define ED25519_FE_ISNEGATIVE_H

#include "fe.h"

/**
 * @brief Returns 1 if the field element is negative (odd after reduction).
 *
 * A field element is considered negative if its least significant bit is 1
 * after full reduction.
 *
 * @param f Input field element.
 * @return 1 if negative, 0 otherwise.
 */
#if ED25519_PLATFORM_64BIT
int fe_isnegative_x64(const fe f);
static inline int fe_isnegative(const fe f)
{
    return fe_isnegative_x64(f);
}
#else
int fe_isnegative_portable(const fe f);
static inline int fe_isnegative(const fe f)
{
    return fe_isnegative_portable(f);
}
#endif

#endif // ED25519_FE_ISNEGATIVE_H
