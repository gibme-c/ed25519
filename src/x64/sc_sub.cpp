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
 * @file x64/sc_sub.cpp
 * @brief x64 implementation of scalar subtraction modulo l using native 64-bit arithmetic.
 */

#include "ed25519_platform.h"
#include "ed25519_secure_erase.h"

#include <cstdint>
#include <cstring>

/* Group order l as 4 x uint64_t (little-endian) */
static const uint64_t L[4] = {
    UINT64_C(0x5812631a5cf5d3ed),
    UINT64_C(0x14def9dea2f79cd6),
    UINT64_C(0x0000000000000000),
    UINT64_C(0x1000000000000000),
};

void sc_sub_x64(unsigned char *s, const unsigned char *a, const unsigned char *b)
{
    uint64_t av[4], bv[4], d[4], r[4];

    std::memcpy(av, a, 32);
    std::memcpy(bv, b, 32);

#if ED25519_HAVE_INT128
    /* d = a - b with borrow */
    __int128 sacc = (__int128)av[0] - bv[0];
    d[0] = (uint64_t)sacc;
    sacc = (__int128)av[1] - bv[1] + (int64_t)(sacc >> 64);
    d[1] = (uint64_t)sacc;
    sacc = (__int128)av[2] - bv[2] + (int64_t)(sacc >> 64);
    d[2] = (uint64_t)sacc;
    sacc = (__int128)av[3] - bv[3] + (int64_t)(sacc >> 64);
    d[3] = (uint64_t)sacc;

    /* Borrow bit: if a < b, sacc >> 64 is -1 (all ones) */
    uint64_t borrow_mask = (uint64_t)((int64_t)(sacc >> 64)); /* 0 or 0xFFFFFFFFFFFFFFFF */

    /* r = d + l (always computed) */
    unsigned __int128 acc = (unsigned __int128)d[0] + L[0];
    r[0] = (uint64_t)acc;
    acc = (unsigned __int128)d[1] + L[1] + (uint64_t)(acc >> 64);
    r[1] = (uint64_t)acc;
    acc = (unsigned __int128)d[2] + L[2] + (uint64_t)(acc >> 64);
    r[2] = (uint64_t)acc;
    acc = (unsigned __int128)d[3] + L[3] + (uint64_t)(acc >> 64);
    r[3] = (uint64_t)acc;
#elif ED25519_HAVE_UMUL128
    /* d = a - b using _subborrow_u64 */
    unsigned char borrow = 0;
    borrow = _subborrow_u64(borrow, av[0], bv[0], &d[0]);
    borrow = _subborrow_u64(borrow, av[1], bv[1], &d[1]);
    borrow = _subborrow_u64(borrow, av[2], bv[2], &d[2]);
    borrow = _subborrow_u64(borrow, av[3], bv[3], &d[3]);

    uint64_t borrow_mask = ~(uint64_t)borrow + 1; /* 0 or 0xFFFFFFFFFFFFFFFF */

    /* r = d + l using _addcarry_u64 */
    unsigned char carry = 0;
    carry = _addcarry_u64(carry, d[0], L[0], &r[0]);
    carry = _addcarry_u64(carry, d[1], L[1], &r[1]);
    carry = _addcarry_u64(carry, d[2], L[2], &r[2]);
    carry = _addcarry_u64(carry, d[3], L[3], &r[3]);
#endif

    /* Constant-time select: if borrow (a < b), result = r (d + l); else result = d */
    d[0] = d[0] ^ (borrow_mask & (d[0] ^ r[0]));
    d[1] = d[1] ^ (borrow_mask & (d[1] ^ r[1]));
    d[2] = d[2] ^ (borrow_mask & (d[2] ^ r[2]));
    d[3] = d[3] ^ (borrow_mask & (d[3] ^ r[3]));

    std::memcpy(s, d, 32);

    ed25519_secure_erase(av, sizeof(av));
    ed25519_secure_erase(bv, sizeof(bv));
    ed25519_secure_erase(d, sizeof(d));
    ed25519_secure_erase(r, sizeof(r));
}
