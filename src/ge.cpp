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

#include "ge.h"

#include "fe_isnonzero.h"
#include "fe_mul.h"
#include "fe_sub.h"

#include <cstddef>

static int ct_memcmp(const void *a, const void *b, size_t len)
{
    const unsigned char *pa = static_cast<const unsigned char *>(a);
    const unsigned char *pb = static_cast<const unsigned char *>(b);
    unsigned char diff = 0;
    for (size_t i = 0; i < len; ++i)
        diff |= pa[i] ^ pb[i];
    return diff;
}

// fe_isnonzero calls fe_tobytes which canonicalizes, so this is safe even
// when a or b carry unreduced limb patterns.
static inline bool fe_eq_ct(const fe a, const fe b)
{
    fe diff;
    fe_sub(diff, a, b);
    return fe_isnonzero(diff) == 0;
}

// Projective equality: two representations are equal iff they denote the same
// affine point regardless of projective scaling. ge_precomp uses byte-literal
// equality because its representation has no projective degree of freedom.

bool ge_p2::operator==(const GeP2 &other) const
{
    fe x1z2, x2z1, y1z2, y2z1;
    fe_mul(x1z2, X, other.Z);
    fe_mul(x2z1, other.X, Z);
    fe_mul(y1z2, Y, other.Z);
    fe_mul(y2z1, other.Y, Z);
    return fe_eq_ct(x1z2, x2z1) && fe_eq_ct(y1z2, y2z1);
}

bool ge_p2::operator!=(const GeP2 &other) const
{
    return !(*this == other);
}

bool ge_p3::operator==(const GeP3 &other) const
{
    // T = X*Y/Z is redundant for well-formed p3, so comparing X/Z and Y/Z suffices.
    fe x1z2, x2z1, y1z2, y2z1;
    fe_mul(x1z2, X, other.Z);
    fe_mul(x2z1, other.X, Z);
    fe_mul(y1z2, Y, other.Z);
    fe_mul(y2z1, other.Y, Z);
    return fe_eq_ct(x1z2, x2z1) && fe_eq_ct(y1z2, y2z1);
}

bool ge_p3::operator!=(const GeP3 &other) const
{
    return !(*this == other);
}

bool ge_p1p1::operator==(const GeP1P1 &other) const
{
    // p1p1 affine form is (X/Z, Y/T).
    fe x1z2, x2z1, y1t2, y2t1;
    fe_mul(x1z2, X, other.Z);
    fe_mul(x2z1, other.X, Z);
    fe_mul(y1t2, Y, other.T);
    fe_mul(y2t1, other.Y, T);
    return fe_eq_ct(x1z2, x2z1) && fe_eq_ct(y1t2, y2t1);
}

bool ge_p1p1::operator!=(const GeP1P1 &other) const
{
    return !(*this == other);
}

bool ge_precomp::operator==(const GePrecomp &other) const
{
    int diff = 0;
    diff |= ct_memcmp(yplusx, other.yplusx, sizeof(yplusx));
    diff |= ct_memcmp(yminusx, other.yminusx, sizeof(yminusx));
    diff |= ct_memcmp(xy2d, other.xy2d, sizeof(xy2d));
    return diff == 0;
}

bool ge_precomp::operator!=(const GePrecomp &other) const
{
    return !(*this == other);
}

bool ge_cached::operator==(const GeCached &other) const
{
    // (YplusX/Z, YminusX/Z) uniquely determines the point; T2d is redundant.
    fe ypx1_z2, ypx2_z1, ymx1_z2, ymx2_z1;
    fe_mul(ypx1_z2, YplusX, other.Z);
    fe_mul(ypx2_z1, other.YplusX, Z);
    fe_mul(ymx1_z2, YminusX, other.Z);
    fe_mul(ymx2_z1, other.YminusX, Z);
    return fe_eq_ct(ypx1_z2, ypx2_z1) && fe_eq_ct(ymx1_z2, ymx2_z1);
}

bool ge_cached::operator!=(const GeCached &other) const
{
    return !(*this == other);
}
