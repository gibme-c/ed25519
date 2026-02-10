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
 * @file x64/ge_dsm_negate_vt_batch_ss_p3.cpp
 * @brief x64 fallback for shared-scalar batch DSM returning ge_p3.
 */

#include "ge.h"
#include "ge_double_scalarmult_negate_vartime.h"
#include "ge_dsm_precomp.h"
#include "ge_p1p1_to_p3.h"

void ge_dsm_negate_vt_batch_ss_p3_x64(
    ge_p3 *results,
    const unsigned char *a,
    const ge_p3 *A_points,
    const unsigned char *b,
    const ge_p3 *B_points,
    size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        ge_dsmp Bi;
        ge_dsm_precomp(Bi, &B_points[i]);
        ge_p1p1 t;
        ge_double_scalarmult_negate_vartime_x64(&t, a, &A_points[i], b, Bi);
        ge_p1p1_to_p3(&results[i], &t);
    }
}
