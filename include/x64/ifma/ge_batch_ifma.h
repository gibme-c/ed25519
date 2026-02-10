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
 * @file ge_batch_ifma.h
 * @brief 8-way parallel group element types and operations for batch scalarmult.
 *
 * This header provides the group element layer on top of fe51x8 field
 * arithmetic -- the curve point types and operations needed to run 8
 * independent scalar multiplications in parallel using AVX-512 IFMA.
 *
 * The point representations mirror the scalar library: ge_p2_8x (projective),
 * ge_p3_8x (extended), ge_p1p1_8x (completed), and ge_cached_8x. Each
 * coordinate is a fe51x8, so a ge_p3_8x holds 8 independent extended points
 * across 20 ZMM registers (4 coordinates x 5 limbs each).
 *
 * The group operation formulas are identical to the scalar ge operations
 * (ge_add.h, ge_sub.h, ge_p2_dbl.h, etc.) and to the AVX2 4-way equivalents
 * in ge_batch_avx2.h. The only IFMA-specific concern is normalization:
 * fe51x8_normalize_weak must be called after any addition that could push
 * a limb above 52 bits (IFMA's silent truncation threshold). In practice,
 * this happens at two specific points:
 *   - ge_add_8x: r->Z = t0 + r->T (where t0 ≤52 bits, r->T ≤51 bits → ≤53)
 *   - ge_sub_8x: r->T = t0 + r->T (same bounds)
 *
 * Table selection uses AVX-512 k-mask comparisons (one 8-bit mask per
 * comparison, one _mm512_mask_blend_epi64 per limb) rather than AVX2's
 * _mm256_blendv_epi8. This is both faster and more natural for 8-way ops.
 */

#ifndef ED25519_X64_IFMA_GE_BATCH_IFMA_H
#define ED25519_X64_IFMA_GE_BATCH_IFMA_H

#include "fe_neg.h"
#include "ge.h"
#include "x64/ifma/fe51x8_ifma.h"

#if defined(_MSC_VER)
#define GE_BATCH_IFMA_FORCE_INLINE __forceinline
#else
#define GE_BATCH_IFMA_FORCE_INLINE inline __attribute__((always_inline))
#endif

// ── 8-way group element types ──
// Same representations as the scalar library, but each coordinate is an fe51x8
// (8 independent field elements). A ge_p3_8x is 20 ZMM registers (4 × 5),
// a ge_cached_8x is also 20 ZMM (YplusX, YminusX, Z, T2d × 5 limbs each).

typedef struct
{
    fe51x8 X, Y, Z;
} ge_p2_8x;
typedef struct
{
    fe51x8 X, Y, Z, T;
} ge_p3_8x;
typedef struct
{
    fe51x8 X, Y, Z, T;
} ge_p1p1_8x;
typedef struct
{
    fe51x8 YplusX, YminusX, Z, T2d;
} ge_cached_8x;

// ── Group element operations (8-way parallel, same formulas as scalar) ──
// These are direct translations of the scalar ge operations from ge_add.h,
// ge_sub.h, ge_p2_dbl.h, etc. The only difference is that every fe_* call
// becomes fe51x8_*, and normalize_weak is inserted where needed.

static GE_BATCH_IFMA_FORCE_INLINE void ge_add_8x(ge_p1p1_8x *r, const ge_p3_8x *p, const ge_cached_8x *q)
{
    fe51x8 t0;
    fe51x8_add(&r->X, &p->Y, &p->X);
    fe51x8_sub(&r->Y, &p->Y, &p->X);
    fe51x8_mul(&r->Z, &r->X, &q->YplusX);
    fe51x8_mul(&r->Y, &r->Y, &q->YminusX);
    fe51x8_mul(&r->T, &q->T2d, &p->T);
    fe51x8_mul(&r->X, &p->Z, &q->Z);
    fe51x8_add(&t0, &r->X, &r->X);
    fe51x8_sub(&r->X, &r->Z, &r->Y);
    fe51x8_add(&r->Y, &r->Z, &r->Y);
    fe51x8_add(&r->Z, &t0, &r->T);
    fe51x8_normalize_weak(&r->Z); // t0 ≤52 + r->T ≤51 → ≤53, must normalize for IFMA
    fe51x8_sub(&r->T, &t0, &r->T);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_sub_8x(ge_p1p1_8x *r, const ge_p3_8x *p, const ge_cached_8x *q)
{
    fe51x8 t0;
    fe51x8_add(&r->X, &p->Y, &p->X);
    fe51x8_sub(&r->Y, &p->Y, &p->X);
    fe51x8_mul(&r->Z, &r->X, &q->YminusX);
    fe51x8_mul(&r->Y, &r->Y, &q->YplusX);
    fe51x8_mul(&r->T, &q->T2d, &p->T);
    fe51x8_mul(&r->X, &p->Z, &q->Z);
    fe51x8_add(&t0, &r->X, &r->X);
    fe51x8_sub(&r->X, &r->Z, &r->Y);
    fe51x8_add(&r->Y, &r->Z, &r->Y);
    fe51x8_sub(&r->Z, &t0, &r->T);
    fe51x8_add(&r->T, &t0, &r->T);
    fe51x8_normalize_weak(&r->T); // t0 ≤52 + r->T ≤51 → ≤53, must normalize for IFMA
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p2_dbl_8x(ge_p1p1_8x *r, const ge_p2_8x *p)
{
    fe51x8 t0;
    fe51x8_sq(&r->X, &p->X);
    fe51x8_sq(&r->Z, &p->Y);
    fe51x8_sq2(&r->T, &p->Z);
    fe51x8_add(&r->Y, &p->X, &p->Y);
    fe51x8_sq(&t0, &r->Y);
    fe51x8_add(&r->Y, &r->Z, &r->X);
    fe51x8_sub(&r->Z, &r->Z, &r->X);
    fe51x8_sub(&r->X, &t0, &r->Y);
    fe51x8_sub(&r->T, &r->T, &r->Z);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p1p1_to_p2_8x(ge_p2_8x *r, const ge_p1p1_8x *p)
{
    fe51x8_mul(&r->X, &p->X, &p->T);
    fe51x8_mul(&r->Y, &p->Y, &p->Z);
    fe51x8_mul(&r->Z, &p->Z, &p->T);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p1p1_to_p3_8x(ge_p3_8x *r, const ge_p1p1_8x *p)
{
    fe51x8_mul(&r->X, &p->X, &p->T);
    fe51x8_mul(&r->Y, &p->Y, &p->Z);
    fe51x8_mul(&r->Z, &p->Z, &p->T);
    fe51x8_mul(&r->T, &p->X, &p->Y);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p3_to_cached_8x(ge_cached_8x *r, const ge_p3_8x *p, const fe51x8 *d2)
{
    fe51x8_add(&r->YplusX, &p->Y, &p->X);
    fe51x8_sub(&r->YminusX, &p->Y, &p->X);
    fe51x8_copy(&r->Z, &p->Z);
    fe51x8_mul(&r->T2d, &p->T, d2);
}

// ── Table selection (8-way, per-lane k-mask) ──
// The batch scalarmult uses a 4-bit fixed-window encoding where each lane
// has its own signed digit at each step. Table selection works by comparing
// the absolute digit against each table index (1..8) to produce a k-mask,
// then blending the matching entry into the accumulator. Each of the 8 lanes
// independently selects its own table entry. After selection, lanes with
// negative digits have their cached point conditionally negated via cneg.

static GE_BATCH_IFMA_FORCE_INLINE void ge_cached_0_8x(ge_cached_8x *t)
{
    fe51x8_1(&t->YplusX);
    fe51x8_1(&t->YminusX);
    fe51x8_1(&t->Z);
    fe51x8_0(&t->T2d);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_cached_cmov_8x(ge_cached_8x *t, const ge_cached_8x *u, __mmask8 mask)
{
    fe51x8_cmov(&t->YplusX, &u->YplusX, mask);
    fe51x8_cmov(&t->YminusX, &u->YminusX, mask);
    fe51x8_cmov(&t->Z, &u->Z, mask);
    fe51x8_cmov(&t->T2d, &u->T2d, mask);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_cached_cneg_8x(ge_cached_8x *t, __mmask8 neg_mask)
{
    // Conditionally swap YplusX and YminusX per lane
    for (int i = 0; i < 5; i++)
    {
        __m512i ypx = t->YplusX.v[i];
        __m512i ymx = t->YminusX.v[i];
        t->YplusX.v[i] = _mm512_mask_blend_epi64(neg_mask, ypx, ymx);
        t->YminusX.v[i] = _mm512_mask_blend_epi64(neg_mask, ymx, ypx);
    }
    // Conditionally negate T2d
    fe51x8 neg_t2d;
    fe51x8_neg(&neg_t2d, &t->T2d);
    fe51x8_cmov(&t->T2d, &neg_t2d, neg_mask);
}

static GE_BATCH_IFMA_FORCE_INLINE void
    ge_p3_blend_8x(ge_p3_8x *dst, const ge_p3_8x *old_val, const ge_p3_8x *new_val, __mmask8 mask)
{
    for (int i = 0; i < 5; i++)
    {
        dst->X.v[i] = _mm512_mask_blend_epi64(mask, old_val->X.v[i], new_val->X.v[i]);
        dst->Y.v[i] = _mm512_mask_blend_epi64(mask, old_val->Y.v[i], new_val->Y.v[i]);
        dst->Z.v[i] = _mm512_mask_blend_epi64(mask, old_val->Z.v[i], new_val->Z.v[i]);
        dst->T.v[i] = _mm512_mask_blend_epi64(mask, old_val->T.v[i], new_val->T.v[i]);
    }
}

static GE_BATCH_IFMA_FORCE_INLINE void
    ge_p2_blend_8x(ge_p2_8x *dst, const ge_p2_8x *old_val, const ge_p2_8x *new_val, __mmask8 mask)
{
    for (int i = 0; i < 5; i++)
    {
        dst->X.v[i] = _mm512_mask_blend_epi64(mask, old_val->X.v[i], new_val->X.v[i]);
        dst->Y.v[i] = _mm512_mask_blend_epi64(mask, old_val->Y.v[i], new_val->Y.v[i]);
        dst->Z.v[i] = _mm512_mask_blend_epi64(mask, old_val->Z.v[i], new_val->Z.v[i]);
    }
}

// ── Pack / Extract: convert between scalar ge types and 8-way types ──
// These are used at the edges of each batch: packing input ge_p3 points
// into ge_p3_8x at entry, and extracting ge_p2 results from ge_p2_8x at
// exit. Since the IFMA batch uses the same radix-2^51 representation as
// the scalar fe type, no limb conversion is needed (unlike the AVX2 batch,
// which must convert between fe51 and fe10).

static GE_BATCH_IFMA_FORCE_INLINE void ge_p3_8x_insert(ge_p3_8x *out, const ge_p3 *p, int lane)
{
    fe51x8_insert_lane(&out->X, p->X, lane);
    fe51x8_insert_lane(&out->Y, p->Y, lane);
    fe51x8_insert_lane(&out->Z, p->Z, lane);
    fe51x8_insert_lane(&out->T, p->T, lane);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p2_8x_extract(ge_p2 *out, const ge_p2_8x *in, int lane)
{
    fe51x8_extract_lane(out->X, &in->X, lane);
    fe51x8_extract_lane(out->Y, &in->Y, lane);
    fe51x8_extract_lane(out->Z, &in->Z, lane);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p3_8x_extract_p3(ge_p3 *out, const ge_p3_8x *in, int lane)
{
    fe51x8_extract_lane(out->X, &in->X, lane);
    fe51x8_extract_lane(out->Y, &in->Y, lane);
    fe51x8_extract_lane(out->Z, &in->Z, lane);
    fe51x8_extract_lane(out->T, &in->T, lane);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p2_0_8x(ge_p2_8x *r)
{
    fe51x8_0(&r->X);
    fe51x8_1(&r->Y);
    fe51x8_1(&r->Z);
}

static GE_BATCH_IFMA_FORCE_INLINE void ge_p3_0_8x(ge_p3_8x *r)
{
    fe51x8_0(&r->X);
    fe51x8_1(&r->Y);
    fe51x8_1(&r->Z);
    fe51x8_0(&r->T);
}

/**
 * @brief Broadcast a scalar ge_cached (fe51) into all 8 lanes of ge_cached_8x.
 */
static GE_BATCH_IFMA_FORCE_INLINE void ge_cached_broadcast_8x(ge_cached_8x *out, const ge_cached *in)
{
    for (int i = 0; i < 5; i++)
    {
        out->YplusX.v[i] = _mm512_set1_epi64((long long)in->YplusX[i]);
        out->YminusX.v[i] = _mm512_set1_epi64((long long)in->YminusX[i]);
        out->Z.v[i] = _mm512_set1_epi64((long long)in->Z[i]);
        out->T2d.v[i] = _mm512_set1_epi64((long long)in->T2d[i]);
    }
}

#endif // ED25519_X64_IFMA_GE_BATCH_IFMA_H
