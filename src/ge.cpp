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

bool ge_p2::operator==(const GeP2 &other) const
{
    int diff = 0;
    diff |= ct_memcmp(X, other.X, sizeof(X));
    diff |= ct_memcmp(Y, other.Y, sizeof(Y));
    diff |= ct_memcmp(Z, other.Z, sizeof(Z));
    return diff == 0;
}

bool ge_p2::operator!=(const GeP2 &other) const
{
    return !(*this == other);
}

bool ge_p3::operator==(const GeP3 &other) const
{
    int diff = 0;
    diff |= ct_memcmp(X, other.X, sizeof(X));
    diff |= ct_memcmp(Y, other.Y, sizeof(Y));
    diff |= ct_memcmp(Z, other.Z, sizeof(Z));
    diff |= ct_memcmp(T, other.T, sizeof(T));
    return diff == 0;
}

bool ge_p3::operator!=(const GeP3 &other) const
{
    return !(*this == other);
}

bool ge_p1p1::operator==(const GeP1P1 &other) const
{
    int diff = 0;
    diff |= ct_memcmp(X, other.X, sizeof(X));
    diff |= ct_memcmp(Y, other.Y, sizeof(Y));
    diff |= ct_memcmp(Z, other.Z, sizeof(Z));
    diff |= ct_memcmp(T, other.T, sizeof(T));
    return diff == 0;
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
    int diff = 0;
    diff |= ct_memcmp(YplusX, other.YplusX, sizeof(YplusX));
    diff |= ct_memcmp(YminusX, other.YminusX, sizeof(YminusX));
    diff |= ct_memcmp(Z, other.Z, sizeof(Z));
    diff |= ct_memcmp(T2d, other.T2d, sizeof(T2d));
    return diff == 0;
}

bool ge_cached::operator!=(const GeCached &other) const
{
    return !(*this == other);
}
