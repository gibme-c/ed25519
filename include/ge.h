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
 * @file ge.h
 * @brief Group element types for the Ed25519 elliptic curve.
 *
 * Points on the Ed25519 curve form a group: you can "add" two points to get
 * a third point, and there's a neutral element (the identity / point at
 * infinity). The curve equation is -x^2 + y^2 = 1 + d*x^2*y^2 where
 * d = -121665/121666 mod p.
 *
 * Rather than storing points as simple (x, y) pairs, we use projective
 * coordinates to avoid expensive field inversions. The library defines
 * several representations, each optimized for different operations:
 *
 * - **ge_p2** (X:Y:Z) - Projective. Cheapest to produce from doubling.
 * - **ge_p3** (X:Y:Z:T) - Extended. The workhorse format for addition.
 *   The extra T = XY/Z coordinate makes additions faster.
 * - **ge_p1p1** (X:Y:Z:T) - Completed. Temporary output from add/double
 *   that needs one more step to become p2 or p3.
 * - **ge_precomp** (y+x, y-x, 2dxy) - For fixed base point table lookups.
 * - **ge_cached** (Y+X, Y-X, Z, 2dT) - For adding the same point repeatedly.
 *
 * The flow is typically: p3 -> cached -> add -> p1p1 -> p3 (for addition)
 * or p2 -> dbl -> p1p1 -> p2 (for doubling).
 */

#ifndef ED25519_GE_H
#define ED25519_GE_H

#include "fe.h"

#include <cstring>
#include <iostream>
#include <string>

/**
 * @brief Projective point representation (X:Y:Z).
 *
 * Represents the affine point (X/Z, Y/Z). Used as the output of point
 * doubling and as an intermediate representation.
 */
typedef struct GeP2
{
    bool operator==(const GeP2 &other) const;

    bool operator!=(const GeP2 &other) const;

    fe X = {0};
    fe Y = {1};
    fe Z = {1};
} ge_p2;

/**
 * @brief Extended point representation (X:Y:Z:T) where T = X*Y/Z.
 *
 * Represents the affine point (X/Z, Y/Z) with the auxiliary coordinate
 * T = X*Y/Z. This is the primary representation used for point addition
 * and the input to most group operations.
 */
typedef struct GeP3
{
    bool operator==(const GeP3 &other) const;

    bool operator!=(const GeP3 &other) const;

    fe X = {0};
    fe Y = {1};
    fe Z = {1};
    fe T = {0};
} ge_p3;

/**
 * @brief Completed point representation (X:Y:Z:T).
 *
 * Represents the affine point ((X/Z), (Y/T)). This is an intermediate
 * result from addition and doubling that must be converted to ge_p2 or
 * ge_p3 before further use.
 */
typedef struct GeP1P1
{
    bool operator==(const GeP1P1 &other) const;

    bool operator!=(const GeP1P1 &other) const;

    fe X = {0};
    fe Y = {1};
    fe Z = {1};
    fe T = {0};
} ge_p1p1;

/**
 * @brief Precomputed point for mixed addition: (y+x, y-x, 2*d*x*y).
 *
 * Used for fixed-base scalar multiplication with the base point table.
 * Stores precomputed values that allow cheaper "mixed" additions.
 */
typedef struct GePrecomp
{
    bool operator==(const GePrecomp &other) const;

    bool operator!=(const GePrecomp &other) const;

    fe yplusx = {1};
    fe yminusx = {1};
    fe xy2d = {0};
} ge_precomp;

/**
 * @brief Cached point for fast addition: (Y+X, Y-X, Z, 2*d*T).
 *
 * Stores precomputed values derived from a ge_p3 point to accelerate
 * repeated additions with the same point.
 */
typedef struct GeCached
{
    bool operator==(const GeCached &other) const;

    bool operator!=(const GeCached &other) const;

    fe YplusX = {1};
    fe YminusX = {1};
    fe Z = {1};
    fe T2d = {0};
} ge_cached;

/**
 * @brief Double-scalar multiplication precomputation table.
 *
 * Stores 8 cached points [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A] for use
 * in variable-time double scalar multiplication.
 */
typedef ge_cached ge_dsmp[8];

namespace std
{
    inline ostream &operator<<(ostream &os, const ge_p2 &value)
    {
        os << "ge_p2: " << std::endl;

        os << "\tX: ";
        for (const auto &val : value.X)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tY: ";
        for (const auto &val : value.Y)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tZ: ";
        for (const auto &val : value.Z)
            os << std::to_string(val) << ",";
        os << std::endl;

        return os;
    }

    inline ostream &operator<<(ostream &os, const ge_p3 &value)
    {
        os << "ge_p3: " << std::endl;

        os << "\tX: ";
        for (const auto &val : value.X)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tY: ";
        for (const auto &val : value.Y)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tZ: ";
        for (const auto &val : value.Z)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tT: ";
        for (const auto &val : value.T)
            os << std::to_string(val) << ",";
        os << std::endl;

        return os;
    }

    inline ostream &operator<<(ostream &os, const ge_p1p1 &value)
    {
        os << "ge_p1p1: " << std::endl;

        os << "\tX: ";
        for (const auto &val : value.X)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tY: ";
        for (const auto &val : value.Y)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tZ: ";
        for (const auto &val : value.Z)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tT: ";
        for (const auto &val : value.T)
            os << std::to_string(val) << ",";
        os << std::endl;

        return os;
    }

    inline ostream &operator<<(ostream &os, const ge_precomp &value)
    {
        os << "ge_precomp: " << std::endl;

        os << "\typlusx: ";
        for (const auto &val : value.yplusx)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tyminusx: ";
        for (const auto &val : value.yminusx)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\txy2d: ";
        for (const auto &val : value.xy2d)
            os << std::to_string(val) << ",";
        os << std::endl;

        return os;
    }

    inline ostream &operator<<(ostream &os, const ge_cached &value)
    {
        os << "ge_cached: " << std::endl;

        os << "\tYplusX: ";
        for (const auto &val : value.YplusX)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tYminusX: ";
        for (const auto &val : value.YminusX)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tZ: ";
        for (const auto &val : value.Z)
            os << std::to_string(val) << ",";
        os << std::endl;

        os << "\tT2d: ";
        for (const auto &val : value.T2d)
            os << std::to_string(val) << ",";
        os << std::endl;

        return os;
    }
} // namespace std

#endif // ED25519_GE_H
