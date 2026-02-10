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
 * @file portable/ge_scalarmult_ct_batch.cpp
 * @brief portable fallback for batch CT scalar multiplication (loop over single ops).
 */

#include "ge.h"
#include "ge_p1p1_to_p2.h"
#include "ge_scalarmult_ct.h"

void ge_scalarmult_ct_batch_portable(ge_p2 *results, const unsigned char *scalars, const ge_p3 *points, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        ge_p1p1 t;
        ge_scalarmult_portable_ct(&t, scalars + i * 32, &points[i]);
        ge_p1p1_to_p2(&results[i], &t);
    }
}
