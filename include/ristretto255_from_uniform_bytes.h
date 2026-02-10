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
 * @file ristretto255_from_uniform_bytes.h
 * @brief Map 64 uniform bytes to a ristretto255 point (hash-to-group).
 *
 * The ristretto255 hash-to-group operation -- takes 64 uniform bytes and
 * produces a ristretto255 point. This is the standard way to derive a curve
 * point from a hash output (e.g. SHA-512) or XOF. Internally, it applies
 * the Elligator 2 map to each 32-byte half independently and adds the two
 * resulting points -- the two-map-then-add construction ensures the output
 * distribution is indistinguishable from uniform over the group.
 *
 * Fully constant-time since the hash output may encode secret data.
 */

#ifndef ED25519_RISTRETTO255_FROM_UNIFORM_BYTES_H
#define ED25519_RISTRETTO255_FROM_UNIFORM_BYTES_H

#include "ge.h"

/**
 * @brief Maps 64 uniform bytes to a ristretto255 point.
 *
 * @param p Output extended point.
 * @param b Input byte array (64 bytes).
 */
void ristretto255_from_uniform_bytes(ge_p3 *p, const unsigned char *b);

#endif // ED25519_RISTRETTO255_FROM_UNIFORM_BYTES_H
