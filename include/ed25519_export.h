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
 * @file ed25519_export.h
 * @brief DLL export/import macro for shared library builds.
 *
 * When building the library as a shared library (DLL/.so/.dylib), public API
 * symbols must be explicitly exported. On Windows this uses __declspec
 * (dllexport when building the library, dllimport when consuming it); on
 * GCC/Clang it uses visibility("default") with hidden default visibility.
 *
 * For static library builds (the default), ED25519_EXPORT expands to nothing.
 *
 * CMake defines:
 * - ED25519_SHARED=1  when BUILD_SHARED_LIBS is ON (PUBLIC, seen by consumers)
 * - ED25519_BUILDING=1  when compiling the library itself (PRIVATE)
 */

#ifndef ED25519_EXPORT_H
#define ED25519_EXPORT_H

#if defined(ED25519_SHARED)
#if defined(_MSC_VER) || defined(__MINGW32__)
#if defined(ED25519_BUILDING)
#define ED25519_EXPORT __declspec(dllexport)
#else
#define ED25519_EXPORT __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define ED25519_EXPORT __attribute__((visibility("default")))
#else
#define ED25519_EXPORT
#endif
#else
#define ED25519_EXPORT
#endif

#endif // ED25519_EXPORT_H
