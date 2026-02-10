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
#include "ed25519_platform.h"
#if !ED25519_PLATFORM_64BIT
#include "ed25519_secure_erase.h"
#include "fe_mul.h"
#include "fe_sq.h"
#include "portable/fe_divpowm1.h"

void fe_divpowm1_portable(fe r, const fe u, const fe v)
{
    fe v3, uv7, t0, t1, t2;
    int i;

    fe_sq(v3, v);
    fe_mul(v3, v3, v); /* v3 = v^3 */
    fe_sq(uv7, v3);
    fe_mul(uv7, uv7, v);
    fe_mul(uv7, uv7, u); /* uv7 = uv^7 */

    fe_sq(t0, uv7);
    fe_sq(t1, t0);
    fe_sq(t1, t1);
    fe_mul(t1, uv7, t1);
    fe_mul(t0, t0, t1);
    fe_sq(t0, t0);
    fe_mul(t0, t1, t0);
    fe_sq(t1, t0);
    for (i = 0; i < 4; ++i)
    {
        fe_sq(t1, t1);
    }
    fe_mul(t0, t1, t0);
    fe_sq(t1, t0);
    for (i = 0; i < 9; ++i)
    {
        fe_sq(t1, t1);
    }
    fe_mul(t1, t1, t0);
    fe_sq(t2, t1);
    for (i = 0; i < 19; ++i)
    {
        fe_sq(t2, t2);
    }
    fe_mul(t1, t2, t1);
    for (i = 0; i < 10; ++i)
    {
        fe_sq(t1, t1);
    }
    fe_mul(t0, t1, t0);
    fe_sq(t1, t0);
    for (i = 0; i < 49; ++i)
    {
        fe_sq(t1, t1);
    }
    fe_mul(t1, t1, t0);
    fe_sq(t2, t1);
    for (i = 0; i < 99; ++i)
    {
        fe_sq(t2, t2);
    }
    fe_mul(t1, t2, t1);
    for (i = 0; i < 50; ++i)
    {
        fe_sq(t1, t1);
    }
    fe_mul(t0, t1, t0);
    fe_sq(t0, t0);
    fe_sq(t0, t0);
    fe_mul(t0, t0, uv7);

    fe_mul(t0, t0, v3);
    fe_mul(r, t0, u);

    ed25519_secure_erase(v3, sizeof(fe));
    ed25519_secure_erase(uv7, sizeof(fe));
    ed25519_secure_erase(t0, sizeof(fe));
    ed25519_secure_erase(t1, sizeof(fe));
    ed25519_secure_erase(t2, sizeof(fe));
}
#endif // !ED25519_PLATFORM_64BIT
