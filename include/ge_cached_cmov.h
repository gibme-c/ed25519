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
 * @file ge_cached_cmov.h
 * @brief Constant-time conditional move for cached points.
 *
 * Applies fe_cmov to each field element of a ge_cached point. Used during
 * constant-time scalar multiplication to select a table entry without
 * revealing which index was chosen via timing or cache side channels.
 */

#ifndef ED25519_GE_CACHED_CMOV_H
#define ED25519_GE_CACHED_CMOV_H

#include "ge.h"

/**
 * @brief Conditionally replaces t with u in constant time.
 *
 * If b is nonzero, sets t = u. If b is zero, t is unchanged.
 *
 * @param t Point to conditionally overwrite.
 * @param u Source point.
 * @param b Condition flag (0 or 1).
 */
#include "fe_cmov.h"

static inline void ge_cached_cmov(ge_cached *t, const ge_cached *u, unsigned char b)
{
    fe_cmov(t->YplusX, u->YplusX, b);
    fe_cmov(t->YminusX, u->YminusX, b);
    fe_cmov(t->Z, u->Z, b);
    fe_cmov(t->T2d, u->T2d, b);
}

#endif // ED25519_GE_CACHED_CMOV_H
