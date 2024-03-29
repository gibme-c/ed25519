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

#include "ge_dsm_precomp.h"

#include "ge_add.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_dbl.h"
#include "ge_p3_to_cached.h"

// adapted from ge_double_scalarmult.c

void ge_dsm_precomp(ge_dsmp r, const ge_p3 *s)
{
    ge_p1p1 t;
    ge_p3 s2, u;
    ge_p3_to_cached(&r[0], s);
    ge_p3_dbl(&t, s);
    ge_p1p1_to_p3(&s2, &t);
    ge_add(&t, &s2, &r[0]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&r[1], &u);
    ge_add(&t, &s2, &r[1]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&r[2], &u);
    ge_add(&t, &s2, &r[2]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&r[3], &u);
    ge_add(&t, &s2, &r[3]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&r[4], &u);
    ge_add(&t, &s2, &r[4]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&r[5], &u);
    ge_add(&t, &s2, &r[5]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&r[6], &u);
    ge_add(&t, &s2, &r[6]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&r[7], &u);
}
