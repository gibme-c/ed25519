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
 * @file x64/sc_check_reduced.cpp
 * @brief x64 implementation of scalar reduction validation via direct comparison.
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

int sc_check_reduced_x64(const unsigned char *s)
{
    uint64_t sv[4];
    std::memcpy(sv, s, 32);

#if ED25519_HAVE_INT128
    /* d = s - l with borrow */
    __int128 sacc = (__int128)sv[0] - L[0];
    sacc = (__int128)sv[1] - L[1] + (int64_t)(sacc >> 64);
    sacc = (__int128)sv[2] - L[2] + (int64_t)(sacc >> 64);
    sacc = (__int128)sv[3] - L[3] + (int64_t)(sacc >> 64);

    /* If borrow (sacc < 0), s < l → valid (return 0). Else s >= l → invalid (return 1). */
    uint64_t borrow_mask = (uint64_t)((int64_t)(sacc >> 64)); /* 0xFFFFFFFFFFFFFFFF if s < l, 0 if s >= l */
    int result = 1 - (int)(borrow_mask >> 63); /* 0 if s < l, 1 if s >= l */
#elif ED25519_HAVE_UMUL128
    uint64_t d;
    unsigned char borrow = 0;
    borrow = _subborrow_u64(borrow, sv[0], L[0], &d);
    borrow = _subborrow_u64(borrow, sv[1], L[1], &d);
    borrow = _subborrow_u64(borrow, sv[2], L[2], &d);
    borrow = _subborrow_u64(borrow, sv[3], L[3], &d);

    /* If borrow, s < l → valid (return 0). Else s >= l → invalid (return 1). */
    int result = 1 - (int)borrow;

    ed25519_secure_erase(&d, sizeof(d));
#endif

    ed25519_secure_erase(sv, sizeof(sv));

    return result;
}
