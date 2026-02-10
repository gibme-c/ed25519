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
#include "ed25519_platform.h"
#if !ED25519_PLATFORM_64BIT
#include "fe_add.h"
#include "fe_invert.h"
#include "fe_mul.h"
#include "fe_sub.h"
#include "fe_tobytes.h"
#include "portable/ge_p3_to_wei25519.h"

// delta = A/3 mod p, where A = 486662 and p = 2^255 - 19
// Limbs in alternating radix-2^26 / radix-2^25 representation:
//   delta = 0x2aaaaaaa_aaaaaaaa_aaaaaaaa_aaaaaaaa_aaaaaaaa_aaaaaaaa_aaaaaaaa_aaad2451
static const fe fe_a_over_3 = {
    44901457,
    11184810,
    22369621,
    22369621,
    44739242,
    11184810,
    22369621,
    22369621,
    44739242,
    11184810,
};

void ge_p3_to_wei25519_portable(unsigned char *s, const ge_p3 *h)
{
    fe num, den, inv, u, x_wei;

    fe_add(num, h->Z, h->Y); // Z + Y
    fe_sub(den, h->Z, h->Y); // Z - Y
    fe_invert(inv, den); // 1/(Z - Y)
    fe_mul(u, num, inv); // u = (Z + Y)/(Z - Y)
    fe_add(x_wei, u, fe_a_over_3); // X_wei = u + A/3
    fe_tobytes(s, x_wei);
}
#endif
