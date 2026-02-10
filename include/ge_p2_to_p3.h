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
 * @file ge_p2_to_p3.h
 * @brief Convert a projective point to extended coordinates.
 *
 * Recovers the T coordinate that ge_p2 doesn't carry. This requires
 * computing T = X*Y/Z, which costs one field multiplication and one
 * inversion -- expensive. Only used when you have a p2 point and need to
 * pass it to an operation that requires the extended form.
 */

#ifndef ED25519_GE_P2_TO_P3_H
#define ED25519_GE_P2_TO_P3_H

#include "ge.h"

/**
 * @brief Converts ge_p2 (projective) to ge_p3 (extended), recomputing T.
 *
 * @param r Output extended point.
 * @param p Input projective point.
 */
#if ED25519_PLATFORM_64BIT
void ge_p2_to_p3_x64(ge_p3 *r, const ge_p2 *p);
static inline void ge_p2_to_p3(ge_p3 *r, const ge_p2 *p)
{
    ge_p2_to_p3_x64(r, p);
}
#else
void ge_p2_to_p3_portable(ge_p3 *r, const ge_p2 *p);
static inline void ge_p2_to_p3(ge_p3 *r, const ge_p2 *p)
{
    ge_p2_to_p3_portable(r, p);
}
#endif

#endif // ED25519_GE_P2_TO_P3_H
