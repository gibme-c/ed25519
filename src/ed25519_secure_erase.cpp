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

#include "ed25519_secure_erase.h"

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__STDC_LIB_EXT1__)
#define ED25519_HAS_MEMSET_S 1
#elif (defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))) || defined(__OpenBSD__) \
    || defined(__FreeBSD__)
#define ED25519_HAS_EXPLICIT_BZERO 1
#include <strings.h>
#else
static void *(*const volatile memset_func)(void *, int, size_t) = std::memset;
#endif

void ed25519_secure_erase(void *pointer, size_t length)
{
    // Zero-length is a no-op; short-circuit before dispatching so callers can
    // safely pass (nullptr, 0). Some underlying primitives (explicit_bzero,
    // memset_s) are declared __attribute__((nonnull)) and trip UBSan on null
    // even when length == 0.
    if (length == 0)
    {
        return;
    }
#ifdef _MSC_VER
    SecureZeroMemory(pointer, length);
#elif defined(ED25519_HAS_MEMSET_S)
    memset_s(pointer, length, 0, length);
#elif defined(ED25519_HAS_EXPLICIT_BZERO)
    explicit_bzero(pointer, length);
#else
    memset_func(pointer, 0, length);
#endif
}
