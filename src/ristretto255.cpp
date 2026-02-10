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
 * @file ristretto255.cpp
 * @brief Ristretto255 group operations per RFC 9496.
 *
 * Ristretto255 defines a prime-order group on top of the Ed25519 curve by
 * mapping equivalence classes of curve points to a unique canonical encoding.
 * This eliminates cofactor-related pitfalls of raw Ed25519 points.
 *
 * All functions are composed of existing fe_* operations which already have
 * platform dispatch, so no x64/portable split is needed here.
 */

#include "fe_0.h"
#include "fe_1.h"
#include "fe_add.h"
#include "fe_cmov.h"
#include "fe_copy.h"
#include "fe_divpowm1.h"
#include "fe_frombytes.h"
#include "fe_isnegative.h"
#include "fe_isnonzero.h"
#include "fe_mul.h"
#include "fe_neg.h"
#include "fe_sq.h"
#include "fe_sub.h"
#include "fe_tobytes.h"
#include "ge.h"
#include "ge_add.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_to_cached.h"

// =============================================================================
// Constants
// =============================================================================

// d = -121665/121666 mod p (Ed25519 curve parameter)
#if ED25519_PLATFORM_64BIT
static const fe fe_d =
    {0x34dca135978a3ULL, 0x1a8283b156ebdULL, 0x5e7a26001c029ULL, 0x739c663a03cbbULL, 0x52036cee2b6ffULL};
#else
static const fe fe_d =
    {-10913610, 13857413, -15372611, 6949391, 114729, -8787816, -6275908, -3247719, -18696448, -12055116};
#endif

// sqrt(-1) mod p
#if ED25519_PLATFORM_64BIT
static const fe fe_sqrtm1 =
    {0x61b274a0ea0b0ULL, 0x0d5a5fc8f189dULL, 0x7ef5e9cbd0c60ULL, 0x78595a6804c9eULL, 0x2b8324804fc1dULL};
#else
static const fe fe_sqrtm1 =
    {-32595792, -7943725, 9377950, 3500415, 12389472, -272473, -25146209, -2005654, 326686, 11406482};
#endif

// sqrt(a*d - 1) where a = -1, i.e. sqrt(-d - 1)
#if ED25519_PLATFORM_64BIT
static const fe SQRT_AD_MINUS_ONE =
    {0x7f6a0497b2e1bULL, 0x1836f0a97afd2ULL, 0x7d747f6be7638ULL, 0x456079e7e6498ULL, 0x376931bf2b834ULL};
#else
static const fe SQRT_AD_MINUS_ONE =
    {24849947, -153582, -23613485, 6347715, -21072328, -667138, -25271143, -15367704, -870347, 14525639};
#endif

// 1/sqrt(a - d) where a = -1, i.e. 1/sqrt(-1 - d)
#if ED25519_PLATFORM_64BIT
static const fe INVSQRT_A_MINUS_D =
    {0x702557fa2bf03ULL, 0x514b7d1a82cc6ULL, 0x7f89efd8b43a7ULL, 0x1aef49ec23700ULL, 0x079376fa30500ULL};
#else
static const fe INVSQRT_A_MINUS_D =
    {-6111485, -4156064, 27798727, -12243468, 25904040, -120897, -20826367, 7060776, -6093568, 1986012};
#endif

// (1 - d^2) mod p
#if ED25519_PLATFORM_64BIT
static const fe ONE_MINUS_D_SQ =
    {0x409c1945fc176ULL, 0x719abc6a1fc4fULL, 0x1c37f90b20684ULL, 0x06bccca55eedfULL, 0x029072a8b2b3eULL};
#else
static const fe ONE_MINUS_D_SQ =
    {6275446, -16617371, -22938544, -3773710, 11667077, 7397348, -27922721, 1766195, -24433858, 672203};
#endif

// (d - 1)^2 mod p
#if ED25519_PLATFORM_64BIT
static const fe D_MINUS_ONE_SQ =
    {0x55aaa44ed4d20ULL, 0x59603c3332635ULL, 0x26d3baf4a7928ULL, 0x120a66e6997a9ULL, 0x5968b37af66c2ULL};
#else
static const fe D_MINUS_ONE_SQ =
    {15551795, -11097455, -13425098, -10125071, -11896535, 10178284, -26634327, 4729244, -5282110, -10116402};
#endif

// =============================================================================
// Internal helpers
// =============================================================================

// Constant-time absolute value: if f is negative, negate it.
static void ct_abs(fe f)
{
    fe neg_f;
    fe_neg(neg_f, f);
    fe_cmov(f, neg_f, (unsigned int)fe_isnegative(f));
}

// Constant-time field element equality: returns 1 if a == b, 0 otherwise.
static int ct_eq(const fe a, const fe b)
{
    fe diff;
    fe_sub(diff, a, b);
    return 1 - (fe_isnonzero(diff) != 0);
}

/**
 * SQRT_RATIO_M1(u, v) -> (was_square, r)
 *
 * Computes r = sqrt(u/v) if it exists, using fe_divpowm1 and corrections
 * via sqrt(-1). Returns was_square = 1 if u/v is a square, 0 otherwise.
 * r is always set to the non-negative root.
 */
static int ristretto255_sqrt_ratio_m1(fe r, const fe u, const fe v)
{
    fe check, v_r2, neg_u, neg_u_sqrtm1, r_prime;

    // r = u * v^3 * (u * v^7)^((p-5)/8)
    fe_divpowm1(r, u, v);

    // check = v * r^2
    fe_sq(v_r2, r);
    fe_mul(check, v, v_r2);

    fe_neg(neg_u, u);
    fe_mul(neg_u_sqrtm1, neg_u, fe_sqrtm1);

    int correct = ct_eq(check, u);
    int flipped = ct_eq(check, neg_u);
    int flipped_i = ct_eq(check, neg_u_sqrtm1);

    fe_mul(r_prime, r, fe_sqrtm1);
    fe_cmov(r, r_prime, (unsigned int)(flipped | flipped_i));

    ct_abs(r);

    return correct | flipped;
}

/**
 * MAP(b) -> ge_p3
 *
 * Elligator 2 map from 32 uniform bytes to a ristretto255 point.
 * Per RFC 9496 Section 4.3.4.
 */
static void ristretto255_map(ge_p3 *p, const unsigned char *b)
{
    fe t, r0, u, v, s, s_prime, c, N, w0, w1, w2, w3;
    fe one, neg_one, tmp, tmp2, s_sq;

    fe_1(one);
    fe_neg(neg_one, one);

    // t = fe_frombytes(b) -- masks MSB, reduces mod p
    fe_frombytes(t, b);

    // r0 = SQRT_M1 * t^2
    fe_sq(tmp, t);
    fe_mul(r0, fe_sqrtm1, tmp);

    // u = (r0 + 1) * ONE_MINUS_D_SQ
    fe_add(tmp, r0, one);
    fe_mul(u, tmp, ONE_MINUS_D_SQ);

    // v = (-1 - r0*d) * (r0 + d)
    fe_mul(tmp, r0, fe_d);
    fe_neg(tmp2, tmp);
    fe_sub(tmp2, tmp2, one); // tmp2 = -1 - r0*d
    fe_add(tmp, r0, fe_d); // tmp = r0 + d
    fe_mul(v, tmp2, tmp);

    // (was_square, s) = SQRT_RATIO_M1(u, v)
    int was_square = ristretto255_sqrt_ratio_m1(s, u, v);

    // s_prime = -|s * t|
    fe_mul(s_prime, s, t);
    ct_abs(s_prime);
    fe_neg(s_prime, s_prime);

    // s = CT_SELECT(s if was_square, else s_prime)
    fe_cmov(s, s_prime, (unsigned int)(1 - was_square));

    // c = CT_SELECT(-1 if was_square, else r0)
    fe_copy(c, neg_one);
    fe_cmov(c, r0, (unsigned int)(1 - was_square));

    // N = c * (r0 - 1) * D_MINUS_ONE_SQ - v
    fe_sub(tmp, r0, one);
    fe_mul(tmp, c, tmp);
    fe_mul(N, tmp, D_MINUS_ONE_SQ);
    fe_sub(N, N, v);

    // s^2
    fe_sq(s_sq, s);

    // w0 = 2 * s * v
    fe_mul(w0, s, v);
    fe_add(w0, w0, w0);

    // w1 = N * SQRT_AD_MINUS_ONE
    fe_mul(w1, N, SQRT_AD_MINUS_ONE);

    // w2 = 1 - s^2
    fe_sub(w2, one, s_sq);

    // w3 = 1 + s^2
    fe_add(w3, one, s_sq);

    // Return (w0*w3, w2*w1, w1*w3, w0*w2)
    fe_mul(p->X, w0, w3);
    fe_mul(p->Y, w2, w1);
    fe_mul(p->Z, w1, w3);
    fe_mul(p->T, w0, w2);
}

// =============================================================================
// Public API
// =============================================================================

/**
 * Decode a 32-byte ristretto255 encoding to an extended point.
 * Per RFC 9496 Section 4.3.1.
 *
 * Returns 0 on success, -1 if the encoding is invalid.
 */
int ristretto255_decode(ge_p3 *p, const unsigned char *s)
{
    fe s_fe, ss, u1, u2, u2_sqr, v, invsqrt, den_x, den_y, x, y, t;
    fe one, tmp;

    // 1. Reject if s is non-canonical (>= p) or IS_NEGATIVE(s)
    // Check s >= p: frombytes reduces mod p, so compare tobytes(frombytes(s)) with s
    fe_frombytes(s_fe, s);
    unsigned char s_check[32];
    fe_tobytes(s_check, s_fe);

    unsigned int diff = 0;
    for (int i = 0; i < 32; i++)
        diff |= s[i] ^ s_check[i];

    if (diff != 0)
        return -1;

    if (fe_isnegative(s_fe))
        return -1;

    fe_1(one);

    // 2. ss = s^2; u1 = 1 - ss; u2 = 1 + ss; u2_sqr = u2^2
    fe_sq(ss, s_fe);
    fe_sub(u1, one, ss);
    fe_add(u2, one, ss);
    fe_sq(u2_sqr, u2);

    // 3. v = -(d * u1^2) - u2_sqr
    fe_sq(tmp, u1);
    fe_mul(v, fe_d, tmp);
    fe_neg(v, v);
    fe_sub(v, v, u2_sqr);

    // 4. (was_square, invsqrt) = SQRT_RATIO_M1(1, v * u2_sqr)
    fe_mul(tmp, v, u2_sqr);
    int was_square = ristretto255_sqrt_ratio_m1(invsqrt, one, tmp);

    // 5. den_x = invsqrt * u2; den_y = invsqrt * den_x * v
    fe_mul(den_x, invsqrt, u2);
    fe_mul(tmp, invsqrt, den_x);
    fe_mul(den_y, tmp, v);

    // 6. x = |2 * s * den_x|; y = u1 * den_y; t = x * y
    fe_mul(x, s_fe, den_x);
    fe_add(x, x, x);
    ct_abs(x);
    fe_mul(y, u1, den_y);
    fe_mul(t, x, y);

    // 7. Reject if !was_square || IS_NEGATIVE(t) || y == 0
    if (!was_square || fe_isnegative(t) || !fe_isnonzero(y))
        return -1;

    // 8. Return (x, y, 1, t)
    fe_copy(p->X, x);
    fe_copy(p->Y, y);
    fe_1(p->Z);
    fe_copy(p->T, t);

    return 0;
}

/**
 * Encode an extended point to 32-byte ristretto255 canonical encoding.
 * Per RFC 9496 Section 4.3.2.
 */
void ristretto255_encode(unsigned char *s, const ge_p3 *p)
{
    fe u1, u2, invsqrt, den1, den2, z_inv, ix, iy, enchanted;
    fe x, y, den_inv, tmp;
    fe one;

    fe_1(one);

    // 1. u1 = (Z+Y)*(Z-Y); u2 = X*Y
    fe_add(tmp, p->Z, p->Y);
    fe u1_tmp;
    fe_sub(u1_tmp, p->Z, p->Y);
    fe_mul(u1, tmp, u1_tmp);
    fe_mul(u2, p->X, p->Y);

    // 2. (_, invsqrt) = SQRT_RATIO_M1(1, u1 * u2^2)
    fe u2_sq;
    fe_sq(u2_sq, u2);
    fe_mul(tmp, u1, u2_sq);
    ristretto255_sqrt_ratio_m1(invsqrt, one, tmp);

    // 3. den1 = invsqrt*u1; den2 = invsqrt*u2; z_inv = den1*den2*T
    fe_mul(den1, invsqrt, u1);
    fe_mul(den2, invsqrt, u2);
    fe_mul(tmp, den1, den2);
    fe_mul(z_inv, tmp, p->T);

    // 4. ix = X*SQRT_M1; iy = Y*SQRT_M1; enchanted = den1*INVSQRT_A_MINUS_D
    fe_mul(ix, p->X, fe_sqrtm1);
    fe_mul(iy, p->Y, fe_sqrtm1);
    fe_mul(enchanted, den1, INVSQRT_A_MINUS_D);

    // 5. rotate = IS_NEGATIVE(T * z_inv)
    fe t_zinv;
    fe_mul(t_zinv, p->T, z_inv);
    unsigned int rotate = (unsigned int)fe_isnegative(t_zinv);

    // 6-8. Conditional select
    fe_copy(x, p->X);
    fe_cmov(x, iy, rotate);

    fe_copy(y, p->Y);
    fe_cmov(y, ix, rotate);

    fe_copy(den_inv, den2);
    fe_cmov(den_inv, enchanted, rotate);

    // 9. y = CT_SELECT(-y if IS_NEGATIVE(x * z_inv), else y)
    fe x_zinv;
    fe_mul(x_zinv, x, z_inv);
    fe neg_y;
    fe_neg(neg_y, y);
    fe_cmov(y, neg_y, (unsigned int)fe_isnegative(x_zinv));

    // 10. s = |den_inv * (Z - y)|
    fe z_minus_y;
    fe_sub(z_minus_y, p->Z, y);
    fe_mul(tmp, den_inv, z_minus_y);
    ct_abs(tmp);

    // 11. Output
    fe_tobytes(s, tmp);
}

/**
 * Constant-time ristretto255 equivalence check.
 * Per RFC 9496 Section 4.3.3.
 *
 * Returns 1 if p and q represent the same ristretto255 element, 0 otherwise.
 */
int ristretto255_equals(const ge_p3 *p, const ge_p3 *q)
{
    fe x1y2, y1x2, y1y2, x1x2;

    fe_mul(x1y2, p->X, q->Y);
    fe_mul(y1x2, p->Y, q->X);
    fe_mul(y1y2, p->Y, q->Y);
    fe_mul(x1x2, p->X, q->X);

    return ct_eq(x1y2, y1x2) | ct_eq(y1y2, x1x2);
}

/**
 * Map 64 uniform bytes to a ristretto255 point.
 * Per RFC 9496 Section 4.3.4.
 *
 * Calls the Elligator 2 map twice on each 32-byte half, then adds.
 */
void ristretto255_from_uniform_bytes(ge_p3 *p, const unsigned char *b)
{
    ge_p3 p0, p1;
    ge_cached p0_cached;
    ge_p1p1 sum;

    ristretto255_map(&p0, b);
    ristretto255_map(&p1, b + 32);

    ge_p3_to_cached(&p0_cached, &p0);
    ge_add(&sum, &p1, &p0_cached);
    ge_p1p1_to_p3(p, &sum);
}
