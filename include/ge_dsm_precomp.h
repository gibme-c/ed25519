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
 * @file ge_dsm_precomp.h
 * @brief Build precomputation table for double scalar multiplication.
 *
 * Builds a table of odd multiples [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A] in
 * cached form for a given point A. The sliding-window double scalar
 * multiplication algorithm looks up these precomputed points by window
 * digit, avoiding expensive recomputations during the main loop.
 */

#ifndef ED25519_GE_DSM_PRECOMP_H
#define ED25519_GE_DSM_PRECOMP_H

#include "ge.h"

/**
 * @brief Builds a table of [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A] for double scalar multiplication.
 *
 * @param r Output precomputation table (8 cached points).
 * @param s Input extended point.
 */
#if ED25519_PLATFORM_64BIT
void ge_dsm_precomp_x64(ge_dsmp r, const ge_p3 *s);
static inline void ge_dsm_precomp(ge_dsmp r, const ge_p3 *s)
{
    ge_dsm_precomp_x64(r, s);
}
#else
void ge_dsm_precomp_portable(ge_dsmp r, const ge_p3 *s);
static inline void ge_dsm_precomp(ge_dsmp r, const ge_p3 *s)
{
    ge_dsm_precomp_portable(r, s);
}
#endif

#endif // ED25519_GE_DSM_PRECOMP_H
