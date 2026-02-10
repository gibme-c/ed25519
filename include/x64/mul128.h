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
#ifndef ED25519_X64_MUL128_H
#define ED25519_X64_MUL128_H

#include "ed25519_platform.h"

#include <cstdint>

#if ED25519_HAVE_INT128

static inline ed25519_uint128 mul64(uint64_t a, uint64_t b)
{
    return (ed25519_uint128)a * b;
}

#elif ED25519_HAVE_UMUL128

struct ed25519_uint128_emu
{
    uint64_t lo;
    uint64_t hi;
};

static inline ed25519_uint128_emu mul64(uint64_t a, uint64_t b)
{
    ed25519_uint128_emu r;
    r.lo = _umul128(a, b, &r.hi);
    return r;
}

static inline ed25519_uint128_emu operator+(ed25519_uint128_emu a, ed25519_uint128_emu b)
{
    ed25519_uint128_emu r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi + (r.lo < a.lo ? 1 : 0);
    return r;
}

static inline ed25519_uint128_emu operator+(ed25519_uint128_emu a, uint64_t b)
{
    ed25519_uint128_emu r;
    r.lo = a.lo + b;
    r.hi = a.hi + (r.lo < a.lo ? 1 : 0);
    return r;
}

static inline ed25519_uint128_emu &operator+=(ed25519_uint128_emu &a, ed25519_uint128_emu b)
{
    a = a + b;
    return a;
}

static inline ed25519_uint128_emu &operator+=(ed25519_uint128_emu &a, uint64_t b)
{
    a = a + b;
    return a;
}

static inline uint64_t shr128(ed25519_uint128_emu v, int shift)
{
    if (shift == 0)
        return v.lo;
    if (shift < 64)
        return (v.lo >> shift) | (v.hi << (64 - shift));
    return v.hi >> (shift - 64);
}

static inline uint64_t lo128(ed25519_uint128_emu v)
{
    return v.lo;
}

#endif // ED25519_HAVE_UMUL128

#endif // ED25519_X64_MUL128_H
