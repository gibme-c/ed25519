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
 * @file ge_p3_dbl.h
 * @brief Point doubling from extended coordinates.
 *
 * Convenience wrapper that drops the T coordinate (p3 -> p2) and then calls
 * ge_p2_dbl. Since doubling doesn't use T, there's no point in keeping it
 * around. The result is a ge_p1p1 that can be converted back to p3 if the T
 * coordinate is needed for a subsequent addition.
 */

#ifndef ED25519_GE_P3_DBL_H
#define ED25519_GE_P3_DBL_H

#include "ge.h"

/**
 * @brief Doubles an extended point: r = 2 * p.
 *
 * Converts to projective internally and calls ge_p2_dbl.
 *
 * @param r Output completed point.
 * @param p Input extended point.
 */
#include "ge_p2_dbl.h"
#include "ge_p3_to_p2.h"

static inline void ge_p3_dbl(ge_p1p1 *r, const ge_p3 *p)
{
    ge_p2 q;
    ge_p3_to_p2(&q, p);
    ge_p2_dbl(r, &q);
}

#endif // ED25519_GE_P3_DBL_H
