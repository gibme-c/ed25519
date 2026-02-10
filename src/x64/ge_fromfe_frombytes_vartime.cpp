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
#include "x64/ge_fromfe_frombytes_vartime.h"

#include "fe_1.h"
#include "fe_add.h"
#include "fe_copy.h"
#include "fe_divpowm1.h"
#include "fe_frombytes.h"
#include "fe_invert.h"
#include "fe_isnegative.h"
#include "fe_isnonzero.h"
#include "fe_mul.h"
#include "fe_neg.h"
#include "fe_sq.h"
#include "fe_sq2.h"
#include "fe_sub.h"

#include <cassert>

static const fe fe_d =
    {0x34dca135978a3ULL, 0x1a8283b156ebdULL, 0x5e7a26001c029ULL, 0x739c663a03cbbULL, 0x52036cee2b6ffULL}; /* d */
static const fe fe_sqrtm1 =
    {0x61b274a0ea0b0ULL, 0x0d5a5fc8f189dULL, 0x7ef5e9cbd0c60ULL, 0x78595a6804c9eULL, 0x2b8324804fc1dULL}; /* sqrt(-1) */
static const fe fe_ma =
    {0x7fffffff892e7ULL, 0x7ffffffffffffULL, 0x7ffffffffffffULL, 0x7ffffffffffffULL, 0x7ffffffffffffULL}; /* -A */
static const fe fe_ma2 =
    {0x7ffc8db3de3c9ULL, 0x7ffffffffffffULL, 0x7ffffffffffffULL, 0x7ffffffffffffULL, 0x7ffffffffffffULL}; /* -A^2 */
static const fe fe_fffb1 = {
    0x76975321c41eeULL,
    0x517254e71a454ULL,
    0x7ec678f465012ULL,
    0x58b9054e29ba0ULL,
    0x7e71fbefdad61ULL}; /* sqrt(-2 * A * (A + 2)) */
static const fe fe_fffb2 = {
    0x66483607c9ae0ULL,
    0x0c08adefbfa5bULL,
    0x0597ef947780dULL,
    0x67b48ea28dbe0ULL,
    0x4d061e0a045a2ULL}; /* sqrt(2 * A * (A + 2)) */
static const fe fe_fffb3 = {
    0x37d8717302c66ULL,
    0x1d4b2c8452b03ULL,
    0x4368bb50093fdULL,
    0x477dc4aa3201fULL,
    0x674a110d14c20ULL}; /* sqrt(-sqrt(-1) * A * (A + 2)) */
static const fe fe_fffb4 = {
    0x51903b6b39186ULL,
    0x11427e94930a7ULL,
    0x3dd0cbbb91bf0ULL,
    0x5fc93607a443fULL,
    0x1a43f3031067dULL}; /* sqrt(sqrt(-1) * A * (A + 2)) */

void ge_fromfe_frombytes_vartime_x64(ge_p2 *r, const unsigned char *s)
{
    fe u, v, w, x, y, z;
    unsigned char sign;

    fe_frombytes(u, s);

    fe_sq2(v, u); /* 2 * u^2 */
    fe_1(w);
    fe_add(w, v, w); /* w = 2 * u^2 + 1 */

    fe_sq(x, w); /* w^2 */
    fe_mul(y, fe_ma2, v); /* -2 * A^2 * u^2 */
    fe_add(x, x, y); /* x = w^2 - 2 * A^2 * u^2 */

    fe_divpowm1(r->X, w, x); /* (w / x)^(m + 1) */

    fe_sq(y, r->X);
    fe_mul(x, y, x);
    fe_sub(y, w, x);
    fe_copy(z, fe_ma);
    if (fe_isnonzero(y))
    {
        fe_add(y, w, x);
        if (fe_isnonzero(y))
        {
            goto negative;
        }
        else
        {
            fe_mul(r->X, r->X, fe_fffb1);
        }
    }
    else
    {
        fe_mul(r->X, r->X, fe_fffb2);
    }
    fe_mul(r->X, r->X, u); /* u * sqrt(2 * A * (A + 2) * w / x) */
    fe_mul(z, z, v); /* -2 * A * u^2 */
    sign = 1;
    goto setsign;
negative:
    fe_mul(x, x, fe_sqrtm1);
    fe_sub(y, w, x);
    if (fe_isnonzero(y))
    {
        assert((fe_add(y, w, x), !fe_isnonzero(y)));
        fe_mul(r->X, r->X, fe_fffb3);
    }
    else
    {
        fe_mul(r->X, r->X, fe_fffb4);
    }
    /* r->X = sqrt(A * (A + 2) * w / x) */
    /* z = -A */
    sign = 0;
setsign:
    if (fe_isnegative(r->X) != sign)
    {
        assert(fe_isnonzero(r->X));
        fe_neg(r->X, r->X);
    }
    fe_add(r->Z, z, w);
    fe_sub(r->Y, z, w);
    fe_mul(r->X, r->X, r->Z);

    fe check_x, check_y, check_iz, check_v;
    fe_invert(check_iz, r->Z);
    fe_mul(check_x, r->X, check_iz);
    fe_mul(check_y, r->Y, check_iz);
    fe_sq(check_x, check_x);
    fe_sq(check_y, check_y);
    fe_mul(check_v, check_x, check_y);
    fe_mul(check_v, fe_d, check_v);
    fe_add(check_v, check_v, check_x);
    fe_sub(check_v, check_v, check_y);
    fe_1(check_x);
    fe_add(check_v, check_v, check_x);
    assert(!fe_isnonzero(check_v));
}
