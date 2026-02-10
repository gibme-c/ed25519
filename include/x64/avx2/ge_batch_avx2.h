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
 * @file ge_batch_avx2.h
 * @brief 4-way parallel group element types and operations for batch scalarmult.
 *
 * This header provides the group element layer on top of fe10x4 field
 * arithmetic -- the curve point types and operations needed to run 4
 * independent scalar multiplications in parallel using AVX2.
 *
 * The point representations mirror the scalar library: ge_p2_4x (projective),
 * ge_p3_4x (extended), ge_p1p1_4x (completed), ge_cached_4x, and
 * ge_precomp_4x. Each coordinate is a fe10x4, so a ge_p3_4x holds 4
 * independent extended points across 40 YMM registers worth of data (4
 * coordinates x 10 limbs each). That's more than the 16 available YMM
 * registers, so the compiler will spill to memory -- this is expected and
 * doesn't meaningfully affect performance since the spills are to L1 cache.
 *
 * The group operation formulas are identical to the scalar fe10 ge operations
 * from the MSVC path of ge_scalarmult_ct.cpp (the radix-2^25.5 versions).
 * No normalization is needed between operations (unlike the IFMA 8-way
 * variant) because fe10x4_mul includes full carry propagation, and the
 * radix-2^25.5 representation doesn't have a silent truncation problem.
 *
 * Table selection uses _mm256_blendv_epi8 with all-ones/all-zeros lane
 * masks from _mm256_cmpeq_epi64. Each of the 4 lanes independently selects
 * from the lookup table, and negative digits are handled by conditional
 * negate (swap YplusX/YminusX and negate T2d or xy2d).
 *
 * Pack/extract operations convert between scalar ge_p3 (fe51) and the 4-way
 * types (fe10x4) via fe51_to_fe10 / fe10_to_fe51 at batch entry and exit.
 */

#ifndef ED25519_X64_AVX2_GE_BATCH_AVX2_H
#define ED25519_X64_AVX2_GE_BATCH_AVX2_H

#include "fe_neg.h"
#include "ge.h"
#include "x64/avx2/fe10x4_avx2.h"

#if defined(_MSC_VER)
#define GE_BATCH_AVX2_FORCE_INLINE __forceinline
#else
#define GE_BATCH_AVX2_FORCE_INLINE inline __attribute__((always_inline))
#endif

// ── 4-way group element types ──
// Same representations as the scalar library, but each coordinate is a fe10x4
// (4 independent field elements in radix-2^25.5). Includes ge_precomp_4x for
// the base-point table in the DSM batch operation.

typedef struct
{
    fe10x4 X, Y, Z;
} ge_p2_4x;
typedef struct
{
    fe10x4 X, Y, Z, T;
} ge_p3_4x;
typedef struct
{
    fe10x4 X, Y, Z, T;
} ge_p1p1_4x;
typedef struct
{
    fe10x4 YplusX, YminusX, Z, T2d;
} ge_cached_4x;
typedef struct
{
    fe10x4 yplusx, yminusx, xy2d;
} ge_precomp_4x;

// ── Group element operations (4-way parallel, same formulas as scalar) ──
// Direct translations of the scalar ge operations from ge_add.h, ge_sub.h,
// ge_madd.h, ge_msub.h, ge_p2_dbl.h, etc. Every fe_* call becomes fe10x4_*.
// Includes madd/msub variants for the base-point table (ge_precomp_4x uses
// the affine Z=1 representation, saving one multiplication per addition).

static GE_BATCH_AVX2_FORCE_INLINE void ge_add_4x(ge_p1p1_4x *r, const ge_p3_4x *p, const ge_cached_4x *q)
{
    fe10x4 t0;
    fe10x4_add(&r->X, &p->Y, &p->X);
    fe10x4_sub(&r->Y, &p->Y, &p->X);
    fe10x4_mul(&r->Z, &r->X, &q->YplusX);
    fe10x4_mul(&r->Y, &r->Y, &q->YminusX);
    fe10x4_mul(&r->T, &q->T2d, &p->T);
    fe10x4_mul(&r->X, &p->Z, &q->Z);
    fe10x4_add(&t0, &r->X, &r->X);
    fe10x4_sub(&r->X, &r->Z, &r->Y);
    fe10x4_add(&r->Y, &r->Z, &r->Y);
    fe10x4_add(&r->Z, &t0, &r->T);
    fe10x4_sub(&r->T, &t0, &r->T);
}

static GE_BATCH_AVX2_FORCE_INLINE void ge_sub_4x(ge_p1p1_4x *r, const ge_p3_4x *p, const ge_cached_4x *q)
{
    fe10x4 t0;
    fe10x4_add(&r->X, &p->Y, &p->X);
    fe10x4_sub(&r->Y, &p->Y, &p->X);
    fe10x4_mul(&r->Z, &r->X, &q->YminusX);
    fe10x4_mul(&r->Y, &r->Y, &q->YplusX);
    fe10x4_mul(&r->T, &q->T2d, &p->T);
    fe10x4_mul(&r->X, &p->Z, &q->Z);
    fe10x4_add(&t0, &r->X, &r->X);
    fe10x4_sub(&r->X, &r->Z, &r->Y);
    fe10x4_add(&r->Y, &r->Z, &r->Y);
    fe10x4_sub(&r->Z, &t0, &r->T);
    fe10x4_add(&r->T, &t0, &r->T);
}

static GE_BATCH_AVX2_FORCE_INLINE void ge_madd_4x(ge_p1p1_4x *r, const ge_p3_4x *p, const ge_precomp_4x *q)
{
    fe10x4 t0;
    fe10x4_add(&r->X, &p->Y, &p->X);
    fe10x4_sub(&r->Y, &p->Y, &p->X);
    fe10x4_mul(&r->Z, &r->X, &q->yplusx);
    fe10x4_mul(&r->Y, &r->Y, &q->yminusx);
    fe10x4_mul(&r->T, &q->xy2d, &p->T);
    fe10x4_add(&t0, &p->Z, &p->Z);
    fe10x4_sub(&r->X, &r->Z, &r->Y);
    fe10x4_add(&r->Y, &r->Z, &r->Y);
    fe10x4_add(&r->Z, &t0, &r->T);
    fe10x4_sub(&r->T, &t0, &r->T);
}

static GE_BATCH_AVX2_FORCE_INLINE void ge_msub_4x(ge_p1p1_4x *r, const ge_p3_4x *p, const ge_precomp_4x *q)
{
    fe10x4 t0;
    fe10x4_add(&r->X, &p->Y, &p->X);
    fe10x4_sub(&r->Y, &p->Y, &p->X);
    fe10x4_mul(&r->Z, &r->X, &q->yminusx);
    fe10x4_mul(&r->Y, &r->Y, &q->yplusx);
    fe10x4_mul(&r->T, &q->xy2d, &p->T);
    fe10x4_add(&t0, &p->Z, &p->Z);
    fe10x4_sub(&r->X, &r->Z, &r->Y);
    fe10x4_add(&r->Y, &r->Z, &r->Y);
    fe10x4_sub(&r->Z, &t0, &r->T);
    fe10x4_add(&r->T, &t0, &r->T);
}

static GE_BATCH_AVX2_FORCE_INLINE void ge_p2_dbl_4x(ge_p1p1_4x *r, const ge_p2_4x *p)
{
    fe10x4 t0;
    fe10x4_sq(&r->X, &p->X);
    fe10x4_sq(&r->Z, &p->Y);
    fe10x4_sq2(&r->T, &p->Z);
    fe10x4_add(&r->Y, &p->X, &p->Y);
    fe10x4_sq(&t0, &r->Y);
    fe10x4_add(&r->Y, &r->Z, &r->X);
    fe10x4_sub(&r->Z, &r->Z, &r->X);
    fe10x4_sub(&r->X, &t0, &r->Y);
    fe10x4_sub(&r->T, &r->T, &r->Z);
}

static GE_BATCH_AVX2_FORCE_INLINE void ge_p1p1_to_p2_4x(ge_p2_4x *r, const ge_p1p1_4x *p)
{
    fe10x4_mul(&r->X, &p->X, &p->T);
    fe10x4_mul(&r->Y, &p->Y, &p->Z);
    fe10x4_mul(&r->Z, &p->Z, &p->T);
}

static GE_BATCH_AVX2_FORCE_INLINE void ge_p1p1_to_p3_4x(ge_p3_4x *r, const ge_p1p1_4x *p)
{
    fe10x4_mul(&r->X, &p->X, &p->T);
    fe10x4_mul(&r->Y, &p->Y, &p->Z);
    fe10x4_mul(&r->Z, &p->Z, &p->T);
    fe10x4_mul(&r->T, &p->X, &p->Y);
}

static GE_BATCH_AVX2_FORCE_INLINE void ge_p3_to_cached_4x(ge_cached_4x *r, const ge_p3_4x *p, const fe10x4 *d2)
{
    fe10x4_add(&r->YplusX, &p->Y, &p->X);
    fe10x4_sub(&r->YminusX, &p->Y, &p->X);
    fe10x4_copy(&r->Z, &p->Z);
    fe10x4_mul(&r->T2d, &p->T, d2);
}

// ── Table selection (4-way, per-lane masking) ──
// The batch scalarmult uses a 4-bit fixed-window encoding where each of the
// 4 lanes has its own signed digit at each step. Table selection compares the
// absolute digit against each table index (1..8) using _mm256_cmpeq_epi64,
// producing a per-lane all-ones/all-zeros mask. The matching entry is blended
// via _mm256_blendv_epi8. After selection, lanes with negative digits have
// their point conditionally negated (swap YplusX/YminusX and negate T2d).

/**
 * @brief 4-way cached zero (identity): YplusX=1, YminusX=1, Z=1, T2d=0.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_cached_0_4x(ge_cached_4x *t)
{
    fe10x4_1(&t->YplusX);
    fe10x4_1(&t->YminusX);
    fe10x4_1(&t->Z);
    fe10x4_0(&t->T2d);
}

/**
 * @brief 4-way precomp zero (identity): yplusx=1, yminusx=1, xy2d=0.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_precomp_0_4x(ge_precomp_4x *t)
{
    fe10x4_1(&t->yplusx);
    fe10x4_1(&t->yminusx);
    fe10x4_0(&t->xy2d);
}

/**
 * @brief 4-way cached conditional move using per-lane mask.
 *
 * mask: all-ones in lanes that should update, all-zeros in others.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_cached_cmov_4x(ge_cached_4x *t, const ge_cached_4x *u, __m256i mask)
{
    fe10x4_cmov(&t->YplusX, &u->YplusX, mask);
    fe10x4_cmov(&t->YminusX, &u->YminusX, mask);
    fe10x4_cmov(&t->Z, &u->Z, mask);
    fe10x4_cmov(&t->T2d, &u->T2d, mask);
}

/**
 * @brief 4-way precomp conditional move using per-lane mask.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_precomp_cmov_4x(ge_precomp_4x *t, const ge_precomp_4x *u, __m256i mask)
{
    fe10x4_cmov(&t->yplusx, &u->yplusx, mask);
    fe10x4_cmov(&t->yminusx, &u->yminusx, mask);
    fe10x4_cmov(&t->xy2d, &u->xy2d, mask);
}

/**
 * @brief 4-way cached conditional negate using per-lane mask.
 *
 * For lanes where mask is all-ones: swap YplusX/YminusX, negate T2d.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_cached_cneg_4x(ge_cached_4x *t, __m256i mask)
{
    // Conditionally swap YplusX and YminusX
    for (int i = 0; i < 10; i++)
    {
        __m256i ypx = t->YplusX.v[i];
        __m256i ymx = t->YminusX.v[i];
        t->YplusX.v[i] = _mm256_blendv_epi8(ypx, ymx, mask);
        t->YminusX.v[i] = _mm256_blendv_epi8(ymx, ypx, mask);
    }
    // Conditionally negate T2d
    fe10x4 neg_t2d;
    fe10x4_neg(&neg_t2d, &t->T2d);
    fe10x4_cmov(&t->T2d, &neg_t2d, mask);
}

/**
 * @brief 4-way precomp conditional negate using per-lane mask.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_precomp_cneg_4x(ge_precomp_4x *t, __m256i mask)
{
    for (int i = 0; i < 10; i++)
    {
        __m256i ypx = t->yplusx.v[i];
        __m256i ymx = t->yminusx.v[i];
        t->yplusx.v[i] = _mm256_blendv_epi8(ypx, ymx, mask);
        t->yminusx.v[i] = _mm256_blendv_epi8(ymx, ypx, mask);
    }
    fe10x4 neg_xy2d;
    fe10x4_neg(&neg_xy2d, &t->xy2d);
    fe10x4_cmov(&t->xy2d, &neg_xy2d, mask);
}

/**
 * @brief 4-way blend for p1p1_to_p3: per-lane select between old u and new t.
 *
 * For lanes where mask is all-ones: use new value. Otherwise keep old.
 */
static GE_BATCH_AVX2_FORCE_INLINE void
    ge_p3_blend_4x(ge_p3_4x *dst, const ge_p3_4x *old_val, const ge_p3_4x *new_val, __m256i mask)
{
    for (int i = 0; i < 10; i++)
    {
        dst->X.v[i] = _mm256_blendv_epi8(old_val->X.v[i], new_val->X.v[i], mask);
        dst->Y.v[i] = _mm256_blendv_epi8(old_val->Y.v[i], new_val->Y.v[i], mask);
        dst->Z.v[i] = _mm256_blendv_epi8(old_val->Z.v[i], new_val->Z.v[i], mask);
        dst->T.v[i] = _mm256_blendv_epi8(old_val->T.v[i], new_val->T.v[i], mask);
    }
}

/**
 * @brief 4-way blend for p1p1_to_p2: per-lane select between old r and new val.
 */
static GE_BATCH_AVX2_FORCE_INLINE void
    ge_p2_blend_4x(ge_p2_4x *dst, const ge_p2_4x *old_val, const ge_p2_4x *new_val, __m256i mask)
{
    for (int i = 0; i < 10; i++)
    {
        dst->X.v[i] = _mm256_blendv_epi8(old_val->X.v[i], new_val->X.v[i], mask);
        dst->Y.v[i] = _mm256_blendv_epi8(old_val->Y.v[i], new_val->Y.v[i], mask);
        dst->Z.v[i] = _mm256_blendv_epi8(old_val->Z.v[i], new_val->Z.v[i], mask);
    }
}

// ── Pack / Extract: convert between scalar ge types and 4-way types ──
// These are used at the edges of each batch: packing input ge_p3 points
// into ge_p3_4x at entry, and extracting ge_p2 results from ge_p2_4x at
// exit. Since the batch uses radix-2^25.5 (fe10x4) but the public API uses
// radix-2^51 (fe), every insert does an fe51_to_fe10 conversion and every
// extract does fe10_to_fe51. This is only a handful of calls per batch, so
// the conversion cost is negligible.

/**
 * @brief Convert ge_p3 (fe51) to fe10 and insert into lane of ge_p3_4x.
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_p3_4x_insert(ge_p3_4x *out, const ge_p3 *p, int lane)
{
    fe10 X10, Y10, Z10, T10;
    fe51_to_fe10(X10, p->X);
    fe51_to_fe10(Y10, p->Y);
    fe51_to_fe10(Z10, p->Z);
    fe51_to_fe10(T10, p->T);
    fe10x4_insert_lane(&out->X, X10, lane);
    fe10x4_insert_lane(&out->Y, Y10, lane);
    fe10x4_insert_lane(&out->Z, Z10, lane);
    fe10x4_insert_lane(&out->T, T10, lane);
}

/**
 * @brief Extract one lane from ge_p2_4x to scalar ge_p2 (fe51).
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_p2_4x_extract(ge_p2 *out, const ge_p2_4x *in, int lane)
{
    fe10 X10, Y10, Z10;
    fe10x4_extract_lane(X10, &in->X, lane);
    fe10x4_extract_lane(Y10, &in->Y, lane);
    fe10x4_extract_lane(Z10, &in->Z, lane);
    fe10_to_fe51(out->X, X10);
    fe10_to_fe51(out->Y, Y10);
    fe10_to_fe51(out->Z, Z10);
}

/**
 * @brief Extract one lane from ge_p3_4x to scalar ge_p3 (fe51).
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_p3_4x_extract_p3(ge_p3 *out, const ge_p3_4x *in, int lane)
{
    fe10 X10, Y10, Z10, T10;
    fe10x4_extract_lane(X10, &in->X, lane);
    fe10x4_extract_lane(Y10, &in->Y, lane);
    fe10x4_extract_lane(Z10, &in->Z, lane);
    fe10x4_extract_lane(T10, &in->T, lane);
    fe10_to_fe51(out->X, X10);
    fe10_to_fe51(out->Y, Y10);
    fe10_to_fe51(out->Z, Z10);
    fe10_to_fe51(out->T, T10);
}

/**
 * @brief Broadcast the d2 constant into all 4 lanes of a fe10x4.
 */
static GE_BATCH_AVX2_FORCE_INLINE void fe10x4_broadcast_d2(fe10x4 *out, const fe d2_fe51)
{
    fe10 d2_10;
    fe51_to_fe10(d2_10, d2_fe51);
    for (int i = 0; i < 10; i++)
        out->v[i] = _mm256_set1_epi64x(d2_10[i]);
}

/**
 * @brief Initialize ge_p2_4x to identity (X=0, Y=1, Z=1).
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_p2_0_4x(ge_p2_4x *r)
{
    fe10x4_0(&r->X);
    fe10x4_1(&r->Y);
    fe10x4_1(&r->Z);
}

/**
 * @brief Initialize ge_p3_4x to identity (X=0, Y=1, Z=1, T=0).
 */
static GE_BATCH_AVX2_FORCE_INLINE void ge_p3_0_4x(ge_p3_4x *r)
{
    fe10x4_0(&r->X);
    fe10x4_1(&r->Y);
    fe10x4_1(&r->Z);
    fe10x4_0(&r->T);
}

#endif // ED25519_X64_AVX2_GE_BATCH_AVX2_H
