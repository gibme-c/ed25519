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
 * @file x25519.cpp
 * @brief X25519 Diffie-Hellman key exchange per RFC 7748.
 *
 * Implements the Montgomery ladder for scalar multiplication on Curve25519
 * in Montgomery form. Platform-agnostic: composed entirely of fe_* operations
 * which already have platform dispatch, so no x64/portable split is needed.
 */

#include "x25519.h"

#include "ed25519_secure_erase.h"
#include "fe_0.h"
#include "fe_1.h"
#include "fe_add.h"
#include "fe_cmov.h"
#include "fe_copy.h"
#include "fe_frombytes.h"
#include "fe_invert.h"
#include "fe_mul.h"
#include "fe_mul121666.h"
#include "fe_sq.h"
#include "fe_sub.h"
#include "fe_tobytes.h"
#include "sc_clamp.h"

#include <cstring>

/**
 * fe_cswap: constant-time conditional swap of two field elements.
 * When b=1, swaps f and g. When b=0, leaves them unchanged.
 * Implemented via fe_cmov with a temporary.
 */
static inline void fe_cswap(fe f, fe g, unsigned int b)
{
    fe temp;
    fe_copy(temp, f);
    fe_cmov(f, g, b);
    fe_cmov(g, temp, b);
}

void x25519(unsigned char shared_secret[32], const unsigned char scalar[32], const unsigned char point[32])
{
    unsigned char e[32];
    std::memcpy(e, scalar, 32);
    sc_clamp(e);

    unsigned char u[32];
    std::memcpy(u, point, 32);
    u[31] &= 0x7f;

    fe x1;
    fe_frombytes(x1, u);

    fe x2, z2, x3, z3;
    fe_1(x2);
    fe_0(z2);
    fe_copy(x3, x1);
    fe_1(z3);

    fe a, aa, bb, cb, da, t, b_val, e_val;

    unsigned int swap = 0;

    for (int pos = 254; pos >= 0; --pos)
    {
        unsigned int b = (e[pos / 8] >> (pos & 7)) & 1;
        swap ^= b;
        fe_cswap(x2, x3, swap);
        fe_cswap(z2, z3, swap);
        swap = b;

        fe_add(a, x2, z2); // A = x2 + z2
        fe_sq(aa, a); // AA = A^2
        fe_sub(b_val, x2, z2); // B = x2 - z2
        fe_sq(bb, b_val); // BB = B^2
        fe_sub(e_val, aa, bb); // E = AA - BB
        fe_add(cb, x3, z3); // C = x3 + z3
        fe_sub(da, x3, z3); // D = x3 - z3
        fe_mul(da, da, a); // DA = D * A
        fe_mul(cb, cb, b_val); // CB = C * B
        fe_add(t, da, cb);
        fe_sq(x3, t); // x3 = (DA + CB)^2
        fe_sub(t, da, cb);
        fe_sq(t, t);
        fe_mul(z3, t, x1); // z3 = x1 * (DA - CB)^2
        fe_mul(x2, aa, bb); // x2 = AA * BB
        fe_mul121666(t, e_val); // t = a24 * E
        fe_add(t, t, bb); // t = BB + a24 * E
        fe_mul(z2, e_val, t); // z2 = E * (BB + a24 * E)
    }

    fe_cswap(x2, x3, swap);
    fe_cswap(z2, z3, swap);

    fe z2_inv;
    fe_invert(z2_inv, z2);
    fe_mul(x2, x2, z2_inv);

    fe_tobytes(shared_secret, x2);

    ed25519_secure_erase(e, sizeof(e));
    ed25519_secure_erase(x1, sizeof(fe));
    ed25519_secure_erase(x2, sizeof(fe));
    ed25519_secure_erase(z2, sizeof(fe));
    ed25519_secure_erase(x3, sizeof(fe));
    ed25519_secure_erase(z3, sizeof(fe));
    ed25519_secure_erase(z2_inv, sizeof(fe));
    ed25519_secure_erase(a, sizeof(fe));
    ed25519_secure_erase(aa, sizeof(fe));
    ed25519_secure_erase(bb, sizeof(fe));
    ed25519_secure_erase(cb, sizeof(fe));
    ed25519_secure_erase(da, sizeof(fe));
    ed25519_secure_erase(t, sizeof(fe));
    ed25519_secure_erase(b_val, sizeof(fe));
    ed25519_secure_erase(e_val, sizeof(fe));
}

void x25519_base(unsigned char public_key[32], const unsigned char scalar[32])
{
    static const unsigned char basepoint[32] = {9};
    x25519(public_key, scalar, basepoint);
}
