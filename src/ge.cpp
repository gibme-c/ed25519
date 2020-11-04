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

bool ge_p2::operator==(const GeP2 &other) const
{
    return memcmp(X, other.X, sizeof(X)) == 0 && memcmp(Y, other.Y, sizeof(Y)) == 0
           && memcmp(Z, other.Z, sizeof(Z)) == 0;
}

bool ge_p2::operator!=(const GeP2 &other) const
{
    return !(*this == other);
}

bool ge_p3::operator==(const GeP3 &other) const
{
    return memcmp(X, other.X, sizeof(X)) == 0 && memcmp(Y, other.Y, sizeof(Y)) == 0
           && memcmp(Z, other.Z, sizeof(Z)) == 0 && memcmp(T, other.T, sizeof(T)) == 0;
}

bool ge_p3::operator!=(const GeP3 &other) const
{
    return !(*this == other);
}

bool ge_p1p1::operator==(const GeP1P1 &other) const
{
    return memcmp(X, other.X, sizeof(X)) == 0 && memcmp(Y, other.Y, sizeof(Y)) == 0
           && memcmp(Z, other.Z, sizeof(Z)) == 0 && memcmp(T, other.T, sizeof(T)) == 0;
}

bool ge_p1p1::operator!=(const GeP1P1 &other) const
{
    return !(*this == other);
}

bool ge_precomp::operator==(const GePrecomp &other) const
{
    return memcmp(yplusx, other.yplusx, sizeof(yplusx)) == 0 && memcmp(yminusx, other.yminusx, sizeof(yminusx)) == 0
           && memcmp(xy2d, other.xy2d, sizeof(xy2d)) == 0;
}

bool ge_precomp::operator!=(const GePrecomp &other) const
{
    return !(*this == other);
}

bool ge_cached::operator==(const GeCached &other) const
{
    return memcmp(YplusX, other.YplusX, sizeof(YplusX)) == 0 && memcmp(YminusX, other.YminusX, sizeof(YminusX)) == 0
           && memcmp(Z, other.Z, sizeof(Z)) == 0 && memcmp(T2d, other.T2d, sizeof(T2d)) == 0;
}

bool ge_cached::operator!=(const GeCached &other) const
{
    return !(*this == other);
}
