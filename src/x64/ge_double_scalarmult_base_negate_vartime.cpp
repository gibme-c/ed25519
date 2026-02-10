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
#include "x64/ge_double_scalarmult_base_negate_vartime.h"

#include "ed25519_secure_erase.h"
#include "ge_add.h"
#include "ge_dsm_precomp.h"
#include "ge_madd.h"
#include "ge_msub.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p2_0.h"
#include "ge_p2_dbl.h"
#include "ge_sub.h"
#include "slide.h"

// adapted from ge25519.c

alignas(64) static const ge_precomp ge_Bi[8] = {
    {{0x493c6f58c3b85ULL, 0x0df7181c325f7ULL, 0x0f50b0b3e4cb7ULL, 0x5329385a44c32ULL, 0x07cf9d3a33d4bULL},
     {0x03905d740913eULL, 0x0ba2817d673a2ULL, 0x23e2827f4e67cULL, 0x133d2e0c21a34ULL, 0x44fd2f9298f81ULL},
     {0x11205877aaa68ULL, 0x479955893d579ULL, 0x50d66309b67a0ULL, 0x2d42d0dbee5eeULL, 0x6f117b689f0c6ULL}},
    {{0x5b0a84cee9730ULL, 0x61d10c97155e4ULL, 0x4059cc8096a10ULL, 0x47a608da8014fULL, 0x7a164e1b9a80fULL},
     {0x11fe8a4fcd265ULL, 0x7bcb8374faaccULL, 0x52f5af4ef4d4fULL, 0x5314098f98d10ULL, 0x2ab91587555bdULL},
     {0x6933f0dd0d889ULL, 0x44386bb4c4295ULL, 0x3cb6d3162508cULL, 0x26368b872a2c6ULL, 0x5a2826af12b9bULL}},
    {{0x2bc4408a5bb33ULL, 0x078ebdda05442ULL, 0x2ffb112354123ULL, 0x375ee8df5862dULL, 0x2945ccf146e20ULL},
     {0x182c3a447d6baULL, 0x22964e536eff2ULL, 0x192821f540053ULL, 0x2f9f19e788e5cULL, 0x154a7e73eb1b5ULL},
     {0x3dbf1812a8285ULL, 0x0fa17ba3f9797ULL, 0x6f69cb49c3820ULL, 0x34d5a0db3858dULL, 0x43aabe696b3bbULL}},
    {{0x25cd0944ea3bfULL, 0x75673b81a4d63ULL, 0x150b925d1c0d4ULL, 0x13f38d9294114ULL, 0x461bea69283c9ULL},
     {0x72c9aaa3221b1ULL, 0x267774474f74dULL, 0x064b0e9b28085ULL, 0x3f04ef53b27c9ULL, 0x1d6edd5d2e531ULL},
     {0x36dc801b8b3a2ULL, 0x0e0a7d4935e30ULL, 0x1deb7cecc0d7dULL, 0x053a94e20dd2cULL, 0x7a9fbb1c6a0f9ULL}},
    {{0x6678aa6a8632fULL, 0x5ea3788d8b365ULL, 0x21bd6d6994279ULL, 0x7ace75919e4e3ULL, 0x34b9ed338add7ULL},
     {0x6217e039d8064ULL, 0x6dea408337e6dULL, 0x57ac112628206ULL, 0x647cb65e30473ULL, 0x49c05a51fadc9ULL},
     {0x4e8bf9045af1bULL, 0x514e33a45e0d6ULL, 0x7533c5b8bfe0fULL, 0x583557b7e14c9ULL, 0x73c172021b008ULL}},
    {{0x700848a802adeULL, 0x1e04605c4e5f7ULL, 0x5c0d01b9767fbULL, 0x7d7889f42388bULL, 0x4275aae2546d8ULL},
     {0x75b0249864348ULL, 0x52ee11070262bULL, 0x237ae54fb5acdULL, 0x3bfd1d03aaab5ULL, 0x18ab598029d5cULL},
     {0x32cc5fd6089e9ULL, 0x426505c949b05ULL, 0x46a18880c7ad2ULL, 0x4a4221888ccdaULL, 0x3dc65522b53dfULL}},
    {{0x0c222a2007f6dULL, 0x356b79bdb77eeULL, 0x41ee81efe12ceULL, 0x120a9bd07097dULL, 0x234fd7eec346fULL},
     {0x7013b327fbf93ULL, 0x1336eeded6a0dULL, 0x2b565a2bbf3afULL, 0x253ce89591955ULL, 0x0267882d17602ULL},
     {0x0a119732ea378ULL, 0x63bf1ba8e2a6cULL, 0x69f94cc90df9aULL, 0x431d1779bfc48ULL, 0x497ba6fdaa097ULL}},
    {{0x6cc0313cfeaa0ULL, 0x1a313848da499ULL, 0x7cb534219230aULL, 0x39596dedefd60ULL, 0x61e22917f12deULL},
     {0x3cd86468ccf0bULL, 0x48553221ac081ULL, 0x6c9464b4e0a6eULL, 0x75fba84180403ULL, 0x43b5cd4218d05ULL},
     {0x2762f9bd0b516ULL, 0x1c6e7fbddcbb3ULL, 0x75909c3ace2bdULL, 0x42101972d3ec9ULL, 0x511d61210ae4dULL}}};

/*
r = a * A + b * B
where a = a[0]+256*a[1]+...+256^31 a[31].
and b = b[0]+256*b[1]+...+256^31 b[31].
B is the Ed25519 base point (x,4/5) with x positive.
*/
void ge_double_scalarmult_base_negate_vartime_x64(
    ge_p1p1 *t,
    const unsigned char *a,
    const ge_p3 *A,
    const unsigned char *b)
{
    signed char aslide[256];
    signed char bslide[256];
    ge_dsmp Ai; /* A, 3A, 5A, 7A, 9A, 11A, 13A, 15A */
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
        ge_p2_dbl_x64(t, &r);

        if (aslide[i] > 0)
        {
            ge_p1p1_to_p3(&u, t);
            ge_add_x64(t, &u, &Ai[aslide[i] / 2]);
        }
        else if (aslide[i] < 0)
        {
            ge_p1p1_to_p3(&u, t);
            ge_sub_x64(t, &u, &Ai[(-aslide[i]) / 2]);
        }

        if (bslide[i] > 0)
        {
            ge_p1p1_to_p3(&u, t);
            ge_madd_x64(t, &u, &ge_Bi[bslide[i] / 2]);
        }
        else if (bslide[i] < 0)
        {
            ge_p1p1_to_p3(&u, t);
            ge_msub_x64(t, &u, &ge_Bi[(-bslide[i]) / 2]);
        }

        ge_p1p1_to_p2(&r, t);
    }

    ed25519_secure_erase(aslide, sizeof(aslide));
    ed25519_secure_erase(bslide, sizeof(bslide));
}
