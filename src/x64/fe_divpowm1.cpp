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
#include "x64/fe_divpowm1.h"

#include "ed25519_secure_erase.h"
#include "x64/fe51_chain.h"

void fe_divpowm1_x64(fe r, const fe u, const fe v)
{
    fe v3, uv7, t0, t1, t2;

    fe51_chain_sq(v3, v);
    fe51_chain_mul(v3, v3, v); /* v3 = v^3 */
    fe51_chain_sq(uv7, v3);
    fe51_chain_mul(uv7, uv7, v);
    fe51_chain_mul(uv7, uv7, u); /* uv7 = uv^7 */

    fe51_chain_sq(t0, uv7);
    fe51_chain_sqn(t1, t0, 2);
    fe51_chain_mul(t1, uv7, t1);
    fe51_chain_mul(t0, t0, t1);
    fe51_chain_sq(t0, t0);
    fe51_chain_mul(t0, t1, t0);
    fe51_chain_sqn(t1, t0, 5);
    fe51_chain_mul(t0, t1, t0);
    fe51_chain_sqn(t1, t0, 10);
    fe51_chain_mul(t1, t1, t0);
    fe51_chain_sqn(t2, t1, 20);
    fe51_chain_mul(t1, t2, t1);
    fe51_chain_sqn(t1, t1, 10);
    fe51_chain_mul(t0, t1, t0);
    fe51_chain_sqn(t1, t0, 50);
    fe51_chain_mul(t1, t1, t0);
    fe51_chain_sqn(t2, t1, 100);
    fe51_chain_mul(t1, t2, t1);
    fe51_chain_sqn(t1, t1, 50);
    fe51_chain_mul(t0, t1, t0);
    fe51_chain_sq(t0, t0);
    fe51_chain_sq(t0, t0);
    fe51_chain_mul(t0, t0, uv7);

    fe51_chain_mul(t0, t0, v3);
    fe51_chain_mul(r, t0, u);

    ed25519_secure_erase(v3, sizeof(fe));
    ed25519_secure_erase(uv7, sizeof(fe));
    ed25519_secure_erase(t0, sizeof(fe));
    ed25519_secure_erase(t1, sizeof(fe));
    ed25519_secure_erase(t2, sizeof(fe));
}
