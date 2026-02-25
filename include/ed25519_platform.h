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
 * @file ed25519_platform.h
 * @brief Compile-time platform detection and 128-bit arithmetic support.
 *
 * This header figures out what the target CPU can do efficiently. On 64-bit
 * platforms (x86_64, ARM64), field elements use a fast radix-2^51
 * representation with 5 limbs per element. On everything else, we fall back
 * to the portable implementation with 10 smaller limbs.
 *
 * It also detects 128-bit multiplication support: GCC/Clang have a native
 * __int128 type, while MSVC provides the _umul128 intrinsic. Field element
 * multiplication needs to compute 64x64->128 bit products, so this matters
 * a lot for performance.
 */

#ifndef ED25519_PLATFORM_H
#define ED25519_PLATFORM_H

#include "ed25519_export.h"

#if defined(__x86_64__) || defined(_M_X64)
#define ED25519_PLATFORM_X64 1
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define ED25519_PLATFORM_ARM64 1
#endif

// Umbrella macro for all 64-bit platforms that use the radix-2^51 representation.
// The "x64" directory and function names are historical; the code is portable C++.
// ED25519_FORCE_PORTABLE overrides to use the 32-bit portable implementation for testing.
#if !ED25519_FORCE_PORTABLE && (ED25519_PLATFORM_X64 || ED25519_PLATFORM_ARM64)
#define ED25519_PLATFORM_64BIT 1
#endif

#if defined(__SIZEOF_INT128__)
#define ED25519_HAVE_INT128 1
typedef unsigned __int128 ed25519_uint128;
#elif defined(_M_X64)
#define ED25519_HAVE_UMUL128 1
#include <intrin.h>
#endif

#endif // ED25519_PLATFORM_H
