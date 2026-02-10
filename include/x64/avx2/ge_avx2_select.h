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
 * @file ge_avx2_select.h
 * @brief AVX2 constant-time table selection helpers for ge_cached and ge_precomp.
 *
 * Replaces the scalar fe_cmov-based ge_cached_cmov / ge_precomp_cmov with
 * AVX2 _mm256_blendv_epi8 operations. Each blend conditionally copies 32 bytes
 * in a single instruction, replacing 4 scalar XOR-mask-XOR chains per fe limb.
 *
 * ge_cached: 4 fe fields × 5 uint64_t = 160 bytes = 5 × 32-byte YMM stores
 * ge_precomp: 3 fe fields × 5 uint64_t = 120 bytes = 3 full + 1 partial
 */

#ifndef ED25519_X64_AVX2_GE_AVX2_SELECT_H
#define ED25519_X64_AVX2_GE_AVX2_SELECT_H

#include "fe_cmov.h"
#include "fe_neg.h"
#include "ge.h"

#include <immintrin.h>

#if defined(_MSC_VER)
#define GE_AVX2_FORCE_INLINE __forceinline
#else
#define GE_AVX2_FORCE_INLINE inline __attribute__((always_inline))
#endif

/**
 * @brief AVX2 constant-time conditional move for ge_cached.
 *
 * If b is nonzero, sets t = u. If b is zero, t is unchanged.
 * Uses 5 × _mm256_blendv_epi8 to cover all 160 bytes.
 */
static GE_AVX2_FORCE_INLINE void ge_cached_cmov_avx2(ge_cached *t, const ge_cached *u, unsigned char b)
{
    // Create a 256-bit mask: all-ones if b!=0, all-zeros if b==0
    const __m256i mask = _mm256_set1_epi8((char)(-(int8_t)b));

    const unsigned char *src = (const unsigned char *)u;
    unsigned char *dst = (unsigned char *)t;

    // 5 × 32-byte blends = 160 bytes = sizeof(ge_cached)
    __m256i d0 = _mm256_loadu_si256((const __m256i *)(dst + 0));
    __m256i s0 = _mm256_loadu_si256((const __m256i *)(src + 0));
    _mm256_storeu_si256((__m256i *)(dst + 0), _mm256_blendv_epi8(d0, s0, mask));

    __m256i d1 = _mm256_loadu_si256((const __m256i *)(dst + 32));
    __m256i s1 = _mm256_loadu_si256((const __m256i *)(src + 32));
    _mm256_storeu_si256((__m256i *)(dst + 32), _mm256_blendv_epi8(d1, s1, mask));

    __m256i d2 = _mm256_loadu_si256((const __m256i *)(dst + 64));
    __m256i s2 = _mm256_loadu_si256((const __m256i *)(src + 64));
    _mm256_storeu_si256((__m256i *)(dst + 64), _mm256_blendv_epi8(d2, s2, mask));

    __m256i d3 = _mm256_loadu_si256((const __m256i *)(dst + 96));
    __m256i s3 = _mm256_loadu_si256((const __m256i *)(src + 96));
    _mm256_storeu_si256((__m256i *)(dst + 96), _mm256_blendv_epi8(d3, s3, mask));

    __m256i d4 = _mm256_loadu_si256((const __m256i *)(dst + 128));
    __m256i s4 = _mm256_loadu_si256((const __m256i *)(src + 128));
    _mm256_storeu_si256((__m256i *)(dst + 128), _mm256_blendv_epi8(d4, s4, mask));
}

/**
 * @brief AVX2 constant-time conditional move for ge_precomp.
 *
 * If b is nonzero, sets t = u. If b is zero, t is unchanged.
 * Uses 3 × _mm256_blendv_epi8 for 96 bytes + scalar blend for remaining 24 bytes.
 */
static GE_AVX2_FORCE_INLINE void ge_precomp_cmov_avx2(ge_precomp *t, const ge_precomp *u, unsigned char b)
{
    const __m256i mask = _mm256_set1_epi8((char)(-(int8_t)b));

    const unsigned char *src = (const unsigned char *)u;
    unsigned char *dst = (unsigned char *)t;

    // 3 × 32-byte blends = 96 bytes
    __m256i d0 = _mm256_loadu_si256((const __m256i *)(dst + 0));
    __m256i s0 = _mm256_loadu_si256((const __m256i *)(src + 0));
    _mm256_storeu_si256((__m256i *)(dst + 0), _mm256_blendv_epi8(d0, s0, mask));

    __m256i d1 = _mm256_loadu_si256((const __m256i *)(dst + 32));
    __m256i s1 = _mm256_loadu_si256((const __m256i *)(src + 32));
    _mm256_storeu_si256((__m256i *)(dst + 32), _mm256_blendv_epi8(d1, s1, mask));

    __m256i d2 = _mm256_loadu_si256((const __m256i *)(dst + 64));
    __m256i s2 = _mm256_loadu_si256((const __m256i *)(src + 64));
    _mm256_storeu_si256((__m256i *)(dst + 64), _mm256_blendv_epi8(d2, s2, mask));

    // Remaining 24 bytes (3 uint64_t): scalar cmov
    uint64_t bmask = 0 - (uint64_t)b;
    uint64_t *d64 = (uint64_t *)(dst + 96);
    const uint64_t *s64 = (const uint64_t *)(src + 96);
    d64[0] ^= bmask & (d64[0] ^ s64[0]);
    d64[1] ^= bmask & (d64[1] ^ s64[1]);
    d64[2] ^= bmask & (d64[2] ^ s64[2]);
}

/**
 * @brief AVX2 constant-time conditional negate for ge_cached.
 *
 * If b is nonzero, swaps YplusX and YminusX, and negates T2d.
 * This computes the negation of a cached point in-place.
 */
static GE_AVX2_FORCE_INLINE void ge_cached_cneg_avx2(ge_cached *t, unsigned char b)
{
    const __m256i mask = _mm256_set1_epi8((char)(-(int8_t)b));

    unsigned char *base = (unsigned char *)t;
    // ge_cached layout: YplusX[0..39], YminusX[40..79], Z[80..119], T2d[120..159]
    // Swap YplusX and YminusX conditionally

    // Load YplusX (first 40 bytes) and YminusX (next 40 bytes)
    // YplusX spans bytes 0-39, YminusX spans bytes 40-79
    // We can blend in 32-byte chunks with some overlap handling

    // Chunk 0: bytes 0-31 of YplusX, bytes 40-71 of YminusX
    __m256i ypx0 = _mm256_loadu_si256((const __m256i *)(base + 0));
    __m256i ymx0 = _mm256_loadu_si256((const __m256i *)(base + 40));
    __m256i new_ypx0 = _mm256_blendv_epi8(ypx0, ymx0, mask);
    __m256i new_ymx0 = _mm256_blendv_epi8(ymx0, ypx0, mask);
    _mm256_storeu_si256((__m256i *)(base + 0), new_ypx0);
    _mm256_storeu_si256((__m256i *)(base + 40), new_ymx0);

    // Remaining 8 bytes of YplusX (bytes 32-39) and YminusX (bytes 72-79): scalar swap
    uint64_t bmask = 0 - (uint64_t)b;
    uint64_t *ypx_tail = (uint64_t *)(base + 32);
    uint64_t *ymx_tail = (uint64_t *)(base + 72);
    uint64_t diff = *ypx_tail ^ *ymx_tail;
    diff &= bmask;
    *ypx_tail ^= diff;
    *ymx_tail ^= diff;

    // Negate T2d conditionally: T2d = b ? -T2d : T2d
    // We compute -T2d using fe_neg semantics, then blend.
    // For simplicity, compute negated T2d on the fly.
    // T2d is at offset 120, 5 × uint64_t.
    // fe_neg(h, f) on x64 uses bias subtraction. We'll use the XOR trick:
    // if b: T2d[i] = neg_T2d[i], else T2d[i] = T2d[i]
    // But negation in radix-2^51 requires bias, not just bit flip.
    // So we need to compute it properly.
    fe neg_t2d;
    fe_neg(neg_t2d, t->T2d);
    fe_cmov(t->T2d, neg_t2d, b);
}

/**
 * @brief AVX2 constant-time conditional negate for ge_precomp.
 *
 * If b is nonzero, swaps yplusx and yminusx, and negates xy2d.
 */
static GE_AVX2_FORCE_INLINE void ge_precomp_cneg_avx2(ge_precomp *t, unsigned char b)
{
    const __m256i mask = _mm256_set1_epi8((char)(-(int8_t)b));

    unsigned char *base = (unsigned char *)t;
    // ge_precomp layout: yplusx[0..39], yminusx[40..79], xy2d[80..119]
    // Swap yplusx and yminusx conditionally

    __m256i ypx0 = _mm256_loadu_si256((const __m256i *)(base + 0));
    __m256i ymx0 = _mm256_loadu_si256((const __m256i *)(base + 40));
    __m256i new_ypx0 = _mm256_blendv_epi8(ypx0, ymx0, mask);
    __m256i new_ymx0 = _mm256_blendv_epi8(ymx0, ypx0, mask);
    _mm256_storeu_si256((__m256i *)(base + 0), new_ypx0);
    _mm256_storeu_si256((__m256i *)(base + 40), new_ymx0);

    // Remaining 8 bytes of each: scalar swap
    uint64_t bmask = 0 - (uint64_t)b;
    uint64_t *ypx_tail = (uint64_t *)(base + 32);
    uint64_t *ymx_tail = (uint64_t *)(base + 72);
    uint64_t diff = *ypx_tail ^ *ymx_tail;
    diff &= bmask;
    *ypx_tail ^= diff;
    *ymx_tail ^= diff;

    // Negate xy2d conditionally
    fe neg_xy2d;
    fe_neg(neg_xy2d, t->xy2d);
    fe_cmov(t->xy2d, neg_xy2d, b);
}

/**
 * @brief AVX2 zero-initialize a ge_cached point.
 */
static GE_AVX2_FORCE_INLINE void ge_cached_0_avx2(ge_cached *t)
{
    // ge_cached identity: YplusX = {1}, YminusX = {1}, Z = {1}, T2d = {0}
    // Just use regular ge_cached_0 — it's trivial and not performance-critical
    // in the table lookup context.
    unsigned char *base = (unsigned char *)t;
    const __m256i zero = _mm256_setzero_si256();
    _mm256_storeu_si256((__m256i *)(base + 0), zero);
    _mm256_storeu_si256((__m256i *)(base + 32), zero);
    _mm256_storeu_si256((__m256i *)(base + 64), zero);
    _mm256_storeu_si256((__m256i *)(base + 96), zero);
    _mm256_storeu_si256((__m256i *)(base + 128), zero);
    // Set identity values: YplusX[0]=1, YminusX[0]=1, Z[0]=1
    t->YplusX[0] = 1;
    t->YminusX[0] = 1;
    t->Z[0] = 1;
}

/**
 * @brief AVX2 zero-initialize a ge_precomp point.
 */
static GE_AVX2_FORCE_INLINE void ge_precomp_0_avx2(ge_precomp *t)
{
    unsigned char *base = (unsigned char *)t;
    const __m256i zero = _mm256_setzero_si256();
    _mm256_storeu_si256((__m256i *)(base + 0), zero);
    _mm256_storeu_si256((__m256i *)(base + 32), zero);
    _mm256_storeu_si256((__m256i *)(base + 64), zero);
    // Zero the remaining 24 bytes (bytes 96-119)
    uint64_t *p = (uint64_t *)(base + 96);
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    // Set identity: yplusx[0]=1, yminusx[0]=1
    t->yplusx[0] = 1;
    t->yminusx[0] = 1;
}

/**
 * @brief fe10 group element selection helpers for the AVX2 fe10-throughout path.
 *
 * These are the fe10 equivalents of the fe51 functions above. A ge_cached_10
 * is 320 bytes (4 coordinates x 10 limbs x 8 bytes = 10 AVX2 blends), and a
 * ge_cached_10 conditional negate has to swap 80-byte YplusX/YminusX fields
 * instead of 40-byte. Used only in the MSVC AVX2 scalarmult TUs where the
 * entire loop runs in fe10 representation.
 */

#include "x64/avx2/fe10_avx2.h"

/**
 * @brief AVX2 zero-initialize a ge_cached_10 to the identity.
 */
static GE_AVX2_FORCE_INLINE void ge_cached_0_10_avx2(ge_cached_10 *t)
{
    unsigned char *base = (unsigned char *)t;
    const __m256i zero = _mm256_setzero_si256();
    // ge_cached_10: 4 × fe10 (10 × int64_t each) = 320 bytes = 10 × 32-byte stores
    _mm256_storeu_si256((__m256i *)(base + 0), zero);
    _mm256_storeu_si256((__m256i *)(base + 32), zero);
    _mm256_storeu_si256((__m256i *)(base + 64), zero);
    _mm256_storeu_si256((__m256i *)(base + 96), zero);
    _mm256_storeu_si256((__m256i *)(base + 128), zero);
    _mm256_storeu_si256((__m256i *)(base + 160), zero);
    _mm256_storeu_si256((__m256i *)(base + 192), zero);
    _mm256_storeu_si256((__m256i *)(base + 224), zero);
    _mm256_storeu_si256((__m256i *)(base + 256), zero);
    _mm256_storeu_si256((__m256i *)(base + 288), zero);
    // Identity: YplusX[0]=1, YminusX[0]=1, Z[0]=1
    t->YplusX[0] = 1;
    t->YminusX[0] = 1;
    t->Z[0] = 1;
}

/**
 * @brief AVX2 constant-time conditional move for ge_cached_10.
 *
 * If b is nonzero, sets t = u. If b is zero, t is unchanged.
 * Uses 10 × _mm256_blendv_epi8 to cover all 320 bytes.
 */
static GE_AVX2_FORCE_INLINE void ge_cached_cmov_10_avx2(ge_cached_10 *t, const ge_cached_10 *u, unsigned char b)
{
    const __m256i mask = _mm256_set1_epi8((char)(-(int8_t)b));

    const unsigned char *src = (const unsigned char *)u;
    unsigned char *dst = (unsigned char *)t;

    for (int i = 0; i < 10; i++)
    {
        __m256i d = _mm256_loadu_si256((const __m256i *)(dst + i * 32));
        __m256i s = _mm256_loadu_si256((const __m256i *)(src + i * 32));
        _mm256_storeu_si256((__m256i *)(dst + i * 32), _mm256_blendv_epi8(d, s, mask));
    }
}

/**
 * @brief AVX2 constant-time conditional negate for ge_cached_10.
 *
 * If b is nonzero, swaps YplusX and YminusX, and negates T2d.
 */
static GE_AVX2_FORCE_INLINE void ge_cached_cneg_10_avx2(ge_cached_10 *t, unsigned char b)
{
    const __m256i mask = _mm256_set1_epi8((char)(-(int8_t)b));

    unsigned char *base = (unsigned char *)t;
    // ge_cached_10 layout: YplusX[0..79], YminusX[80..159], Z[160..239], T2d[240..319]
    // Each fe10 = 10 × int64_t = 80 bytes

    // Swap YplusX and YminusX: 80 bytes = 2 full AVX2 loads + 1 partial (16 bytes)
    // But for simplicity and correctness, use 2.5 × 32-byte blends
    __m256i ypx0 = _mm256_loadu_si256((const __m256i *)(base + 0));
    __m256i ymx0 = _mm256_loadu_si256((const __m256i *)(base + 80));
    _mm256_storeu_si256((__m256i *)(base + 0), _mm256_blendv_epi8(ypx0, ymx0, mask));
    _mm256_storeu_si256((__m256i *)(base + 80), _mm256_blendv_epi8(ymx0, ypx0, mask));

    __m256i ypx1 = _mm256_loadu_si256((const __m256i *)(base + 32));
    __m256i ymx1 = _mm256_loadu_si256((const __m256i *)(base + 112));
    _mm256_storeu_si256((__m256i *)(base + 32), _mm256_blendv_epi8(ypx1, ymx1, mask));
    _mm256_storeu_si256((__m256i *)(base + 112), _mm256_blendv_epi8(ymx1, ypx1, mask));

    // Remaining 16 bytes of each (bytes 64-79 and 144-159): scalar swap
    uint64_t bmask = 0 - (uint64_t)b;
    uint64_t *ypx_tail = (uint64_t *)(base + 64);
    uint64_t *ymx_tail = (uint64_t *)(base + 144);
    for (int i = 0; i < 2; i++)
    {
        uint64_t diff = ypx_tail[i] ^ ymx_tail[i];
        diff &= bmask;
        ypx_tail[i] ^= diff;
        ymx_tail[i] ^= diff;
    }

    // Negate T2d conditionally (at offset 240, 80 bytes)
    fe10 neg_t2d;
    fe10_neg(neg_t2d, t->T2d);
    fe10_cmov(t->T2d, neg_t2d, (int64_t)b);
}

#endif // ED25519_X64_AVX2_GE_AVX2_SELECT_H
