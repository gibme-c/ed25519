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
 * @file ristretto255_decode.h
 * @brief Decode a 32-byte ristretto255 encoding to an extended point.
 *
 * Takes 32 bytes and recovers a ge_p3 point -- the ristretto255 counterpart
 * to ge_frombytes_vartime for raw Ed25519 points. Unlike raw point decoding,
 * this enforces the ristretto255 canonical form: every valid encoding maps to
 * exactly one equivalence class, so there's no cofactor ambiguity to worry
 * about. Invalid or non-canonical encodings are rejected.
 *
 * The computation itself is constant-time (no branches on intermediate values);
 * only the final accept/reject decision branches, which is safe since the
 * input is untrusted public data.
 */

#ifndef ED25519_RISTRETTO255_DECODE_H
#define ED25519_RISTRETTO255_DECODE_H

#include "ge.h"

/**
 * @brief Decodes a 32-byte ristretto255 encoding to a ge_p3 point.
 *
 * @param p Output extended point.
 * @param s Input byte array (32 bytes).
 * @return 0 on success, -1 if the encoding is invalid.
 */
int ristretto255_decode(ge_p3 *p, const unsigned char *s);

#endif // ED25519_RISTRETTO255_DECODE_H
