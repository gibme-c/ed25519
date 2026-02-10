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
 * @file ed25519_secure_erase.h
 * @brief Secure memory erasure that the compiler won't optimize away.
 *
 * When you're done with secret data (private keys, nonces, intermediate
 * scalar values), you need to zero the memory. A plain memset can be
 * removed by the compiler if it can prove the buffer isn't read afterward.
 * This function uses platform-specific tricks (SecureZeroMemory on MSVC/Windows,
 * memset_s on C11, explicit_bzero on glibc/OpenBSD/FreeBSD, volatile function
 * pointer to memset elsewhere) to guarantee the zeroing actually happens.
 */

#ifndef ED25519_SECURE_ERASE_H
#define ED25519_SECURE_ERASE_H

#include <cstring>

void ed25519_secure_erase(void *pointer, size_t length);

#endif