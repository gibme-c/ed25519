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
 * @file ristretto255.h
 * @brief Master include for ristretto255 group operations.
 *
 * Ristretto255 (RFC 9496) builds a prime-order group on top of the Ed25519
 * curve. The problem it solves: Ed25519 has a cofactor of 8, which means
 * multiple curve points can represent the "same" group element, leading to
 * subtle bugs in protocols that assume a prime-order group. Ristretto maps
 * equivalence classes of curve points to a unique canonical encoding, giving
 * you a clean prime-order abstraction without changing the underlying curve.
 *
 * Including this header pulls in all four ristretto255 operations:
 *
 * - **Decode**: 32 bytes to internal ge_p3 point
 * - **Encode**: ge_p3 point to 32 bytes (canonical)
 * - **Equals**: constant-time ristretto equivalence check
 * - **From uniform bytes**: 64 bytes to ristretto point (hash-to-group)
 */

#ifndef ED25519_RISTRETTO255_H
#define ED25519_RISTRETTO255_H

#include "ristretto255_decode.h"
#include "ristretto255_encode.h"
#include "ristretto255_equals.h"
#include "ristretto255_from_uniform_bytes.h"

#endif // ED25519_RISTRETTO255_H
