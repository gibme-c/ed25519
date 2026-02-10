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
 * @file sc.h
 * @brief Scalar utility includes for Ed25519 scalar operations.
 *
 * Scalars are 256-bit integers modulo the group order
 * l = 2^252 + 27742317777372353535851937790883648493. That's the number of
 * points in the prime-order subgroup of the Ed25519 curve. Private keys,
 * nonces, and signature components are all scalars.
 *
 * Scalars are stored as 32 bytes in little-endian order. This header pulls
 * in the byte-loading helpers (load_3, load_4) used internally by the
 * scalar arithmetic functions.
 */

#ifndef ED25519_SC_H
#define ED25519_SC_H

#include "load_3.h"
#include "load_4.h"

#endif // ED25519_SC_H
