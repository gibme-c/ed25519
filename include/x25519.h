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
 * @file x25519.h
 * @brief X25519 Diffie-Hellman key exchange (RFC 7748).
 *
 * X25519 is the Diffie-Hellman function on Curve25519 in Montgomery form.
 * Given a scalar (private key) and a u-coordinate (public key or basepoint),
 * it computes the shared secret via a constant-time Montgomery ladder.
 *
 * Usage for key exchange:
 *   1. Generate a random 32-byte private key.
 *   2. Compute public key: x25519_base(pub, priv).
 *   3. Exchange public keys with peer.
 *   4. Compute shared secret: x25519(shared, my_priv, peer_pub).
 *   5. Both parties derive the same shared secret.
 *   6. Zero private key and shared secret after use via ed25519_secure_erase().
 */

#ifndef ED25519_X25519_H
#define ED25519_X25519_H

/**
 * @brief Compute X25519 Diffie-Hellman: shared_secret = scalar * point.
 *
 * Performs scalar multiplication on the Montgomery curve Curve25519 using the
 * constant-time Montgomery ladder (RFC 7748 §5). The scalar is clamped per
 * RFC 7748 before use.
 *
 * @param shared_secret Output 32-byte shared secret (u-coordinate).
 * @param scalar        Input 32-byte scalar (private key).
 * @param point         Input 32-byte u-coordinate (peer's public key).
 */
void x25519(unsigned char shared_secret[32], const unsigned char scalar[32], const unsigned char point[32]);

/**
 * @brief Compute X25519 public key: public_key = scalar * basepoint(9).
 *
 * Thin wrapper around x25519() with the standard basepoint u=9.
 *
 * @param public_key Output 32-byte public key (u-coordinate).
 * @param scalar     Input 32-byte scalar (private key).
 */
void x25519_base(unsigned char public_key[32], const unsigned char scalar[32]);

#endif // ED25519_X25519_H
