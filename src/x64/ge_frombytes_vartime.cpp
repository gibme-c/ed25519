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
#include "x64/ge_frombytes_vartime.h"

#include "fe_1.h"
#include "fe_add.h"
#include "fe_divpowm1.h"
#include "fe_frombytes.h"
#include "fe_isnegative.h"
#include "fe_isnonzero.h"
#include "fe_mul.h"
#include "fe_neg.h"
#include "fe_sq.h"
#include "fe_sub.h"

static const fe fe_d =
    {0x34dca135978a3ULL, 0x1a8283b156ebdULL, 0x5e7a26001c029ULL, 0x739c663a03cbbULL, 0x52036cee2b6ffULL}; /* d */
static const fe fe_sqrtm1 =
    {0x61b274a0ea0b0ULL, 0x0d5a5fc8f189dULL, 0x7ef5e9cbd0c60ULL, 0x78595a6804c9eULL, 0x2b8324804fc1dULL}; /* sqrt(-1) */

int ge_frombytes_vartime_x64(ge_p3 *h, const unsigned char *s)
{
    fe u;
    fe v;
    fe vxx;
    fe check;

    /* Reject non-canonical y (y >= p = 2^255-19) */
    {
        unsigned int c = (s[31] & 0x7f) ^ 0x7f;
        for (int i = 30; i >= 1; i--)
            c |= s[i] ^ 0xff;
        if (c == 0 && s[0] >= 0xed)
            return -1;
    }

    fe_frombytes(h->Y, s);
    fe_1(h->Z);
    fe_sq(u, h->Y);
    fe_mul(v, u, fe_d);
    fe_sub(u, u, h->Z); /* u = y^2-1 */
    fe_add(v, v, h->Z); /* v = dy^2+1 */

    fe_divpowm1(h->X, u, v); /* x = uv^3(uv^7)^((q-5)/8) */

    fe_sq(vxx, h->X);
    fe_mul(vxx, vxx, v);
    fe_sub(check, vxx, u); /* vx^2-u */
    if (fe_isnonzero(check))
    {
        fe_add(check, vxx, u); /* vx^2+u */
        if (fe_isnonzero(check))
        {
            return -1;
        }
        fe_mul(h->X, h->X, fe_sqrtm1);
    }

    /* If x = 0, the sign must be positive */
    if (!fe_isnonzero(h->X) && (s[31] >> 7))
    {
        return -1;
    }

    if (fe_isnegative(h->X) != (s[31] >> 7))
    {
        fe_neg(h->X, h->X);
    }

    fe_mul(h->T, h->X, h->Y);
    return 0;
}
