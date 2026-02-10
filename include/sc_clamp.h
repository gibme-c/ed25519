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
 * @file sc_clamp.h
 * @brief RFC 8032 scalar clamping.
 *
 * Clamping tweaks a raw 32-byte scalar (typically the hash of a private key)
 * to ensure safe use with the Ed25519 curve. Three things happen:
 *
 * - Clear bits 0-2: makes the scalar a multiple of 8 (the cofactor), which
 *   prevents small-subgroup attacks in Diffie-Hellman.
 * - Clear bit 255: ensures the scalar fits in 255 bits (since l ~ 2^252,
 *   this avoids potential issues with oversize scalars).
 * - Set bit 254: ensures the scalar's high bit is in a fixed position, which
 *   gives the scalar multiplication a constant number of doublings and makes
 *   certain timing attacks harder.
 */

#ifndef ED25519_SC_CLAMP_H
#define ED25519_SC_CLAMP_H

/**
 * @brief Applies RFC 8032 clamping to a 32-byte scalar in place.
 *
 * Clears bits 0-2, clears bit 255, and sets bit 254.
 *
 * @param s Input/output 32-byte scalar.
 */
static inline void sc_clamp(unsigned char *s)
{
    s[0] &= 248;
    s[31] &= 63;
    s[31] |= 64;
}

#endif // ED25519_SC_CLAMP_H
