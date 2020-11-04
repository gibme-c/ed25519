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

#include "ge_scalarmult.h"

#if defined __SIZEOF_INT128__ && defined __USE_64BIT__

#include "ge_add.h"
#include "ge_frombytes_negate_vartime.h"
#include "ge_p3_tobytes.h"

void donna128_scalarmult_wrapper(ge_p1p1 *r, const unsigned char *a, const ge_p3 *A)
{
    uint8_t result_bytes[32];

    uint8_t point[32];

    // convert the p3 to bytes as donna128 works with bytes
    ge_p3_tobytes(point, A);

    // perform the calc
    donna128_scalarmult(result_bytes, a, point);

    ge_p3 result_p3;

    // donna returns bytes, but we need a p3
    ge_frombytes_negate_vartime(&result_p3, result_bytes);

    ge_p1p1 result_p1p1;

    ge_cached zero;

    // quick hack -- add a zero point to the point to get a p1p1
    ge_add(r, &result_p3, &zero);
}
#endif
