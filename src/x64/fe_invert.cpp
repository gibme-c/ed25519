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
#include "x64/fe_invert.h"

#include "ed25519_secure_erase.h"
#include "x64/fe51_chain.h"

void fe_invert_x64(fe out, const fe z)
{
    fe t0;
    fe t1;
    fe t2;
    fe t3;

    /* z^(2^1) */
    fe51_chain_sq(t0, z);
    /* z^(2^2) */
    fe51_chain_sq(t1, t0);
    /* z^(2^3) */
    fe51_chain_sq(t1, t1);
    /* z^(2^3 + 2^0) = z^9 */
    fe51_chain_mul(t1, z, t1);
    /* z^(2^3 + 2^1 + 2^0) = z^11 */
    fe51_chain_mul(t0, t0, t1);
    /* z^(2^4 + 2^2 + 2^1) */
    fe51_chain_sq(t2, t0);
    /* z^(2^4 + 2^2 + 2^1 + 2^3 + 2^0) */
    fe51_chain_mul(t1, t1, t2);
    /* z^(2^9 + ...) */
    fe51_chain_sqn(t2, t1, 5);
    fe51_chain_mul(t1, t2, t1);
    /* z^(2^19 + ...) */
    fe51_chain_sqn(t2, t1, 10);
    fe51_chain_mul(t2, t2, t1);
    /* z^(2^39 + ...) */
    fe51_chain_sqn(t3, t2, 20);
    fe51_chain_mul(t2, t3, t2);
    /* z^(2^49 + ...) */
    fe51_chain_sqn(t2, t2, 10);
    fe51_chain_mul(t1, t2, t1);
    /* z^(2^99 + ...) */
    fe51_chain_sqn(t2, t1, 50);
    fe51_chain_mul(t2, t2, t1);
    /* z^(2^199 + ...) */
    fe51_chain_sqn(t3, t2, 100);
    fe51_chain_mul(t2, t3, t2);
    /* z^(2^249 + ...) */
    fe51_chain_sqn(t2, t2, 50);
    fe51_chain_mul(t1, t2, t1);
    /* z^(2^254 + ...) */
    fe51_chain_sqn(t1, t1, 5);
    /* z^(p-2) */
    fe51_chain_mul(out, t1, t0);

    ed25519_secure_erase(t0, sizeof(fe));
    ed25519_secure_erase(t1, sizeof(fe));
    ed25519_secure_erase(t2, sizeof(fe));
    ed25519_secure_erase(t3, sizeof(fe));

    return;
}
