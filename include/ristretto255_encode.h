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
 * @file ristretto255_encode.h
 * @brief Encode an extended point to 32-byte ristretto255 canonical form.
 *
 * Serializes a ge_p3 point to 32 bytes in ristretto255 canonical form -- the
 * counterpart to ge_tobytes/ge_p3_tobytes for raw Ed25519 points. The key
 * property: all points in the same ristretto255 equivalence class (related by
 * the cofactor or sign) produce the same 32-byte output. This is what makes
 * ristretto255 useful -- you get a unique encoding without having to think
 * about cofactor pitfalls.
 *
 * Fully constant-time since the input point may be derived from secret data.
 */

#ifndef ED25519_RISTRETTO255_ENCODE_H
#define ED25519_RISTRETTO255_ENCODE_H

#include "ge.h"

/**
 * @brief Encodes a ge_p3 point to 32-byte ristretto255 canonical form.
 *
 * @param s Output byte array (32 bytes).
 * @param p Input extended point.
 */
void ristretto255_encode(unsigned char *s, const ge_p3 *p);

#endif // ED25519_RISTRETTO255_ENCODE_H
