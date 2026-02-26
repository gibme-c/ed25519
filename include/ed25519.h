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
 * @file ed25519.h
 * @brief Master include header for the Ed25519 library.
 *
 * Ed25519 is an elliptic curve digital signature algorithm. It works on the
 * twisted Edwards curve -x^2 + y^2 = 1 + d*x^2*y^2 over the prime field
 * GF(2^255 - 19). The library is organized in three layers:
 *
 * - **Field elements (fe_*)**: Arithmetic modulo p = 2^255 - 19. These are
 *   the coordinates of points on the curve.
 * - **Group elements (ge_*)**: Points on the curve and operations like
 *   addition, doubling, and scalar multiplication.
 * - **Scalars (sc_*)**: 256-bit integers modulo the group order l, used as
 *   private keys and in signature equations.
 *
 * Including this header pulls in everything. You can also include individual
 * headers (e.g. fe_mul.h, ge_scalarmult_ct.h) if you only need specific ops.
 *
 * @note **This is a low-level cryptographic primitive library.** It does NOT
 * provide high-level signing or verification functions, nor a SHA-512
 * implementation. Callers implementing Ed25519 signing per RFC 8032 must:
 *
 * 1. Supply their own SHA-512 implementation.
 * 2. Derive the nonce deterministically: r = SHA-512(prefix || message).
 * 3. Validate signature S < l via sc_check_reduced() before verification.
 * 4. Validate public keys are on the curve via ge_frombytes_vartime().
 * 5. Check public key subgroup membership via
 *    ge_check_subgroup_precomp_vartime() when required by the protocol.
 * 6. Use ge_scalarmult_base_ct() / ge_scalarmult_ct() for secret scalar
 *    operations (constant-time), and _vartime functions only for
 *    verification on public data.
 * 7. Zero sensitive data after use via ed25519_secure_erase().
 */

#ifndef ED25519_H
#define ED25519_H

#include "benchmark.h"
#include "ed25519_cpuid.h"
#include "ed25519_dispatch.h"
#include "fe.h"
#include "fe_0.h"
#include "fe_1.h"
#include "fe_add.h"
#include "fe_cmov.h"
#include "fe_copy.h"
#include "fe_divpowm1.h"
#include "fe_frombytes.h"
#include "fe_invert.h"
#include "fe_isnegative.h"
#include "fe_isnonzero.h"
#include "fe_mul.h"
#include "fe_mul121666.h"
#include "fe_neg.h"
#include "fe_pow22523.h"
#include "fe_sq.h"
#include "fe_sq2.h"
#include "fe_sub.h"
#include "fe_tobytes.h"
#include "ge.h"
#include "ge_add.h"
#include "ge_cached_0.h"
#include "ge_cached_cmov.h"
#include "ge_check_subgroup_precomp_negate_vartime.h"
#include "ge_check_subgroup_precomp_vartime.h"
#include "ge_double_scalarmult_base_negate_vartime.h"
#include "ge_double_scalarmult_base_negate_vartime_batch.h"
#include "ge_double_scalarmult_negate_vartime.h"
#include "ge_double_scalarmult_negate_vartime_batch.h"
#include "ge_double_scalarmult_negate_vartime_batch_ss.h"
#include "ge_double_scalarmult_negate_vartime_batch_ss_p3.h"
#include "ge_dsm_precomp.h"
#include "ge_frombytes_vartime.h"
#include "ge_fromfe_frombytes_vartime.h"
#include "ge_madd.h"
#include "ge_msub.h"
#include "ge_mul8.h"
#include "ge_multiscalar_mul_vartime.h"
#include "ge_p1p1_to_p2.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p2_0.h"
#include "ge_p2_dbl.h"
#include "ge_p2_to_p3.h"
#include "ge_p3_0.h"
#include "ge_p3_dbl.h"
#include "ge_p3_to_cached.h"
#include "ge_p3_to_p2.h"
#include "ge_p3_to_wei25519.h"
#include "ge_p3_tobytes.h"
#include "ge_precomp_0.h"
#include "ge_precomp_cmov.h"
#include "ge_scalarmult_base_ct.h"
#include "ge_scalarmult_ct.h"
#include "ge_scalarmult_ct_batch.h"
#include "ge_sub.h"
#include "ge_tobytes.h"
#include "ristretto255.h"
#include "sc.h"
#include "sc_0.h"
#include "sc_add.h"
#include "sc_check_clamped.h"
#include "sc_check_reduced.h"
#include "sc_clamp.h"
#include "sc_isnonzero.h"
#include "sc_mul.h"
#include "sc_muladd.h"
#include "sc_mulsub.h"
#include "sc_reduce.h"
#include "sc_sub.h"
#include "x25519.h"

#endif // ED25519_H
