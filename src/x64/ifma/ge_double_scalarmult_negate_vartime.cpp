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
 * @file ifma/ge_double_scalarmult_negate_vartime.cpp
 * @brief AVX-512 IFMA variable-time double scalar multiplication.
 *
 * Same algorithm as the x64 scalar path but uses IFMA field arithmetic.
 */

#include "ed25519_secure_erase.h"
#include "fe_add.h"
#include "fe_copy.h"
#include "fe_sub.h"
#include "ge.h"
#include "ge_dsm_precomp.h"
#include "ge_p2_0.h"
#include "slide.h"
#include "x64/ifma/fe_ifma_chain.h"

// IFMA ge operations inlined here

static FE_IFMA_FORCE_INLINE void ge_add_ifma(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe_ifma_chain_mul_nn(r->Z, r->X, q->YplusX);
    fe_ifma_chain_mul_nn(r->Y, r->Y, q->YminusX);
    fe_ifma_chain_mul_nn(r->T, q->T2d, p->T);
    fe_ifma_chain_mul_nn(r->X, p->Z, q->Z);
    fe_add(t0, r->X, r->X);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_add(r->Z, t0, r->T);
    fe_normalize_weak(r->Z); // t0 ≤52 + r->T ≤51 → ≤53, must normalize for IFMA
    fe_sub(r->T, t0, r->T);
}

static FE_IFMA_FORCE_INLINE void ge_sub_ifma(ge_p1p1 *r, const ge_p3 *p, const ge_cached *q)
{
    fe t0;
    fe_add(r->X, p->Y, p->X);
    fe_sub(r->Y, p->Y, p->X);
    fe_ifma_chain_mul_nn(r->Z, r->X, q->YminusX);
    fe_ifma_chain_mul_nn(r->Y, r->Y, q->YplusX);
    fe_ifma_chain_mul_nn(r->T, q->T2d, p->T);
    fe_ifma_chain_mul_nn(r->X, p->Z, q->Z);
    fe_add(t0, r->X, r->X);
    fe_sub(r->X, r->Z, r->Y);
    fe_add(r->Y, r->Z, r->Y);
    fe_sub(r->Z, t0, r->T);
    fe_add(r->T, t0, r->T);
    fe_normalize_weak(r->T); // t0 ≤52 + r->T ≤51 → ≤53, must normalize for IFMA
}

static FE_IFMA_FORCE_INLINE void ge_p2_dbl_ifma(ge_p1p1 *r, const ge_p2 *p)
{
    fe t0;
    fe_ifma_chain_sq_n(r->X, p->X);
    fe_ifma_chain_sq_n(r->Z, p->Y);
    fe_ifma_chain_sq2_n(r->T, p->Z);
    fe_add(r->Y, p->X, p->Y);
    fe_ifma_chain_sq_n(t0, r->Y);
    fe_add(r->Y, r->Z, r->X);
    fe_sub(r->Z, r->Z, r->X);
    fe_sub(r->X, t0, r->Y);
    fe_sub(r->T, r->T, r->Z);
}

static FE_IFMA_FORCE_INLINE void ge_p1p1_to_p2_ifma(ge_p2 *r, const ge_p1p1 *p)
{
    fe_ifma_chain_mul_nn(r->X, p->X, p->T);
    fe_ifma_chain_mul_nn(r->Y, p->Y, p->Z);
    fe_ifma_chain_mul_nn(r->Z, p->Z, p->T);
}

static FE_IFMA_FORCE_INLINE void ge_p1p1_to_p3_ifma(ge_p3 *r, const ge_p1p1 *p)
{
    fe_ifma_chain_mul_nn(r->X, p->X, p->T);
    fe_ifma_chain_mul_nn(r->Y, p->Y, p->Z);
    fe_ifma_chain_mul_nn(r->Z, p->Z, p->T);
    fe_ifma_chain_mul_nn(r->T, p->X, p->Y);
}

/*
r = a * A + b * B
where a = a[0]+256*a[1]+...+256^31 a[31].
and b = b[0]+256*b[1]+...+256^31 b[31].
*/
void ge_double_scalarmult_negate_vartime_ifma(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b,
    const ge_dsmp Bi)
{
    signed char aslide[256];
    signed char bslide[256];
    alignas(64) ge_dsmp Ai; /* A, 3A, 5A, 7A, 9A, 11A, 13A, 15A */
    ge_p3 u;
    int i;

    slide(aslide, a);
    slide(bslide, b);

    ge_dsm_precomp(Ai, A);

    ge_p2 r;

    ge_p2_0(&r);

    for (i = 255; i >= 0; --i)
    {
        if (aslide[i] || bslide[i])
            break;
    }

    for (; i >= 0; --i)
    {
        ge_p2_dbl_ifma(t, &r);

        if (aslide[i] > 0)
        {
            ge_p1p1_to_p3_ifma(&u, t);
            ge_add_ifma(t, &u, &Ai[aslide[i] / 2]);
        }
        else if (aslide[i] < 0)
        {
            ge_p1p1_to_p3_ifma(&u, t);
            ge_sub_ifma(t, &u, &Ai[(-aslide[i]) / 2]);
        }

        if (bslide[i] > 0)
        {
            ge_p1p1_to_p3_ifma(&u, t);
            ge_add_ifma(t, &u, &Bi[bslide[i] / 2]);
        }
        else if (bslide[i] < 0)
        {
            ge_p1p1_to_p3_ifma(&u, t);
            ge_sub_ifma(t, &u, &Bi[(-bslide[i]) / 2]);
        }

        ge_p1p1_to_p2_ifma(&r, t);
    }

    ed25519_secure_erase(aslide, sizeof(aslide));
    ed25519_secure_erase(bslide, sizeof(bslide));
}
