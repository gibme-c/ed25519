# ed25519

[![CI Build Tests](https://github.com/gibme-c/ed25519/actions/workflows/ci.yml/badge.svg)](https://github.com/gibme-c/ed25519/actions/workflows/ci.yml)

A C++ static library for [Ed25519](https://en.wikipedia.org/wiki/EdDSA#Ed25519) elliptic curve cryptography primitives, based on the SUPERCOP ref10 reference implementation.

Ed25519 is a high-speed, high-security elliptic curve used for digital signatures and key exchange. It operates on the twisted Edwards curve **-x² + y² = 1 + d·x²·y²** over the prime field **GF(2²⁵⁵ - 19)** (hence the name). This library provides the low-level building blocks — field arithmetic, curve point operations, scalar math, and ristretto255 — so you can build protocols on top without worrying about the math underneath.

**This is not a signature library.** There's no `sign()` or `verify()` function here. Instead, you get the primitives that those operations are built from: constant-time scalar multiplication, multi-scalar multiplication, hash-to-curve, subgroup checking, and so on. If you need a turnkey Ed25519 signature scheme, look at [libsodium](https://doc.libsodium.org/) or [OpenSSL](https://www.openssl.org/). If you need to *build* a protocol — Schnorr signatures, VRFs, Bulletproofs, threshold schemes — this library gives you the building blocks with full control over the math.

## Features

### Core Primitives

- **Field arithmetic** over GF(2²⁵⁵ - 19) — add, subtract, multiply, square, invert, and more
- **Curve point operations** — projective, extended, completed, and precomputed representations for efficient computation
- **Scalar arithmetic** modulo the group order *l* (~2²⁵²)

### Scalar Multiplication

- **Constant-time scalar multiplication** — both fixed-base and variable-base, with no secret-dependent branches or memory access patterns
- **Variable-time double scalar multiplication** for signature verification
- **Multi-scalar multiplication** — compute s₁·P₁ + s₂·P₂ + ... + sₙ·Pₙ in a single pass using Straus (n≤32) and Pippenger (n>32) algorithms, with logarithmic speedup over serial
- **Batch scalar multiplication** — process N independent operations in parallel via AVX2 (4-way) and AVX-512 IFMA (8-way) horizontal SIMD

### Curve Utilities

- **Cofactor handling** — `ge_mul8` clears the cofactor, `sc_clamp` applies RFC 8032 bit clamping
- **Hash-to-curve** — Elligator-like mapping from arbitrary bytes to curve points (`ge_fromfe_frombytes_vartime`)
- **Subgroup checking** — verify points lie in the prime-order subgroup
- **Curve form conversion** — `ge_p3_to_wei25519` extracts the Wei25519 (short-Weierstrass) X-coordinate from an Ed25519 point, bridging Edwards and Weierstrass representations

### Higher-Level Abstractions

- **Ristretto255** (RFC 9496) — prime-order group abstraction over Ed25519, with encode, decode, equality, and hash-to-group operations

### Performance and Security

- **SIMD acceleration** (x86_64) — runtime-dispatched AVX2 and AVX-512 IFMA backends for constant-time scalar multiplication and verification, with automatic CPU feature detection via CPUID
- **Secure memory erasure** — `ed25519_secure_erase` zeros secret data using platform-specific mechanisms the compiler can't optimize away
- **Cross-platform** — MSVC, GCC, Clang, AppleClang, MinGW

## Building

Requires CMake 3.10+ and a C++17 compiler.

```bash
# Configure and build
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Release -j

# Run tests
./build/ed25519-tests          # Linux / macOS
./build/Release/ed25519-tests  # Windows (MSVC)
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | `OFF` | Build the unit tests (`ed25519-tests`) |
| `BUILD_BENCHMARKS` | `OFF` | Build the benchmark tool (`ed25519-benchmark`) |
| `FORCE_PORTABLE` | `OFF` | Force the 32-bit portable implementation on 64-bit platforms (for testing/comparison) |
| `ARCH` | `native` | Target CPU architecture for `-march` (`native`, `default`, or a specific arch) |
| `ENABLE_LTO` | `OFF` | Enable link-time optimization |
| `ENABLE_AVX2` | `ON`* | Enable AVX2 SIMD backend with runtime dispatch |
| `ENABLE_AVX512` | `ON`* | Enable AVX-512 IFMA SIMD backend with runtime dispatch |
| `BUILD_TOOLS` | `OFF` | Build the test vector generator (`ed25519-gen-testvectors`) |
| `CMAKE_BUILD_TYPE` | `Release` | `Debug`, `Release`, or `RelWithDebInfo` |

\* On x86_64 only. Both default to `OFF` on other architectures or when `FORCE_PORTABLE` is set. Set either to `OFF` to benchmark individual code paths in isolation.

## Usage

Include the master header to access all operations:

```cpp
#include "ed25519.h"
```

Or include individual headers for specific operations:

```cpp
#include "fe_mul.h"
#include "ge_scalarmult_base_ct.h"
#include "sc_reduce.h"
```

Link against the `ed25519` static library target in your CMake project:

```cmake
add_subdirectory(ed25519)
target_link_libraries(your_target ed25519)
```

### Quick Example

A minimal Schnorr-style signature using the library primitives:

```cpp
#include "ed25519.h"
#include <cstring>

// Compute public key: A = clamp(seed) · B
void keypair(const unsigned char seed[32], unsigned char public_key[32]) {
    unsigned char scalar[32];
    memcpy(scalar, seed, 32);
    sc_clamp(scalar);

    ge_p3 A;
    ge_scalarmult_base_ct(&A, scalar);
    ge_p3_tobytes(public_key, &A);

    ed25519_secure_erase(scalar, 32);
}

// Verify: check that [s]B - [h]A == R
bool verify(const ge_p3 *A, const unsigned char R[32],
            const unsigned char s[32], const unsigned char h[32]) {
    ge_p2 check;
    ge_double_scalarmult_base_negate_vartime(&check, h, A, s);

    unsigned char result[32];
    ge_tobytes(result, &check);
    return memcmp(result, R, 32) == 0;
}
```

On x86_64, initialize SIMD dispatch before any crypto operations:

```cpp
// Fast heuristic — picks IFMA > AVX2 > baseline
ed25519_init();

// Or: benchmark all backends, pick fastest per-function (~1-2 seconds)
ed25519_init(true);
```

Both modes are thread-safe and only execute once; subsequent calls are no-ops.

## Architecture

The library is organized in three layers, matching the math of elliptic curve cryptography:

1. **Field elements (`fe_*`)** — Integers modulo p = 2²⁵⁵ - 19. These are the coordinates of curve points.
2. **Group elements (`ge_*`)** — Points on the Ed25519 curve. Addition, doubling, scalar multiplication, encoding/decoding.
3. **Scalars (`sc_*`)** — 256-bit integers modulo the group order *l*. Used as private keys and in signature equations.

### Platform Dispatch

The library uses two levels of dispatch to get the best performance on each platform without sacrificing portability.

**Compile-time dispatch** via `ed25519_platform.h` selects the field element representation:

- **64-bit** (x86_64, ARM64): Field elements are `uint64_t[5]` in radix-2⁵¹. Multiplication uses 128-bit products (`__int128` on GCC/Clang, `_umul128` on MSVC).
- **Everything else**: Falls back to the portable implementation with `int32_t[10]` in alternating 26/25-bit limbs.

Both representations are 40 bytes, so the `fe` type has the same layout regardless of backend. The `FORCE_PORTABLE` CMake option forces the 32-bit path on 64-bit platforms for testing.

**Runtime dispatch** (x86_64 only) selects SIMD-accelerated implementations for the eight hot-path scalar multiplication functions:

- `ge_scalarmult_ct` — variable-base, constant-time
- `ge_scalarmult_base_ct` — fixed-base, constant-time
- `ge_double_scalarmult_negate_vartime` — verification
- `ge_double_scalarmult_base_negate_vartime` — verification
- `ge_scalarmult_ct_batch` — batch variable-base, constant-time
- `ge_double_scalarmult_base_negate_vartime_batch` — batch verification
- `ge_double_scalarmult_negate_vartime_batch_ss` — batch DSM with shared scalars (returns `ge_p2`)
- `ge_double_scalarmult_negate_vartime_batch_ss_p3` — batch DSM with shared scalars (returns `ge_p3`, avoids expensive p2-to-p3 conversion)

CPU features (AVX2, AVX-512F, AVX-512 IFMA) are detected via CPUID+XGETBV at startup. Call `ed25519_init()` for fast heuristic selection, or `ed25519_init(true)` to benchmark all available implementations and pick the fastest per-function.

### SIMD Backends (x86_64)

Two SIMD backends accelerate the constant-time scalar multiplication and verification operations.

**AVX2** uses `_mm256_blendv_epi8` for branchless constant-time table selection (both MSVC and GCC). On MSVC, the entire scalar multiplication loop runs in radix-2²⁵·⁵ (`fe10`) representation to avoid uint128 struct emulation overhead, converting to/from radix-2⁵¹ only at entry and exit. On GCC/Clang, the standard radix-2⁵¹ chain ops are used (since `__int128` is already efficient) with AVX2 cmov for table lookups. Batch operations use 4-way horizontal parallelism via `fe10x4` (10 x `__m256i`), processing 4 independent operations per batch iteration.

**AVX-512 IFMA** uses `vpmadd52lo`/`vpmadd52hi` for field element multiplication, replacing the multi-instruction schoolbook multiply with hardware 52-bit fused multiply-accumulate. This backend includes a lazy normalization optimization: most IFMA operations skip input carry-propagation when bit-width bounds are known safe, with `fe_normalize_weak` inserted only at the few points where limbs could exceed 52 bits. Batch operations use 8-way horizontal parallelism via `fe51x8` (5 x `__m512i`), processing 8 independent operations per batch iteration. IFMA dispatch is enabled for all compilers when AVX-512 IFMA is detected.

### Force-Inline System

Performance-critical group element operations use a **chain macro** pattern to control inlining of field element mul/sq within larger operations:

- **GCC/Clang**: Field element multiply and square are force-inlined directly into group element bodies for maximum optimization.
- **MSVC**: Field element ops stay as extern calls because MSVC's optimizer regresses when inlining large bodies that involve its uint128 emulation structs.

This split is handled automatically by the `fe51_chain.h`, `fe25_chain.h`, `fe10_avx2_chain.h`, and `fe_ifma_chain.h` headers.

## API Reference

### Field Elements (`fe_*`)

Arithmetic on elements of GF(2²⁵⁵ - 19). These are the coordinates of points on the curve — every point operation ultimately boils down to field element math.

- **Arithmetic**: `fe_add`, `fe_sub`, `fe_mul`, `fe_sq`, `fe_sq2`, `fe_neg`
- **Higher-order**: `fe_invert`, `fe_pow22523`, `fe_divpowm1`
- **Serialization**: `fe_frombytes`, `fe_tobytes`
- **Utilities**: `fe_copy`, `fe_cmov`, `fe_0`, `fe_1`, `fe_isnegative`, `fe_isnonzero`

### Group Elements (`ge_*`)

Points on the Ed25519 curve. Multiple internal representations (projective, extended, completed, precomputed) are used for efficient computation — conversions between them are provided so you can chain operations.

**Point types**: `ge_p2` (projective), `ge_p3` (extended), `ge_p1p1` (completed), `ge_precomp`, `ge_cached`

**Arithmetic**: `ge_add`, `ge_sub`, `ge_madd`, `ge_msub`, `ge_p2_dbl`, `ge_p3_dbl`, `ge_mul8`

**Scalar multiplication**:
- `ge_scalarmult_base_ct` — fixed-base, constant-time
- `ge_scalarmult_ct` — variable-base, constant-time

**Batch operations**:
- `ge_scalarmult_ct_batch` — N independent variable-base scalarmults
- `ge_double_scalarmult_base_negate_vartime_batch` — N independent verification scalarmults
- `ge_double_scalarmult_negate_vartime_batch` — N independent general DSM
- `ge_double_scalarmult_negate_vartime_batch_ss` — N DSM with shared scalars, returns `ge_p2`
- `ge_double_scalarmult_negate_vartime_batch_ss_p3` — same but returns `ge_p3` directly, avoiding expensive p2-to-p3 inversion

**Multi-scalar multiplication**:
- `ge_multiscalar_mul_vartime` — Q = Σ sᵢ·Pᵢ
- `ge_multiscalar_mul_base_vartime` — Q = s₀·B + Σ sᵢ·Pᵢ

**Verification**: `ge_double_scalarmult_negate_vartime`, `ge_double_scalarmult_base_negate_vartime`

**Serialization**: `ge_tobytes`, `ge_p3_tobytes`, `ge_frombytes_vartime`

**Curve form conversion**: `ge_p3_to_wei25519` — extracts the Wei25519 short-Weierstrass X-coordinate from a `ge_p3` point, using the birational map X_wei = (Z+Y)/(Z-Y) + A/3 where A=486662

**Hash-to-curve**: `ge_fromfe_frombytes_vartime`

### Scalars (`sc_*`)

Operations on 32-byte scalars modulo the group order *l*. These are the secret exponents and signature components.

- **Arithmetic**: `sc_add`, `sc_sub`, `sc_mul`, `sc_muladd`, `sc_mulsub`
- **Reduction**: `sc_reduce(s, len)` — modular reduction mod *l* (supports 32-byte or 64-byte input)
- **Clamping**: `sc_clamp` — RFC 8032 bit clamping (clears bits 0-2 and 255, sets bit 254)
- **Validation**: `sc_check_reduced` (checks s < *l*), `sc_check_clamped` (checks RFC 8032 clamping), `sc_isnonzero`, `sc_0`

### Ristretto255 (`ristretto255_*`)

Prime-order group abstraction over the Ed25519 curve ([RFC 9496](https://www.rfc-editor.org/rfc/rfc9496.html)). Ristretto maps equivalence classes of curve points to a unique canonical 32-byte encoding, eliminating cofactor-related pitfalls. All operations work with the same `ge_p3` point type used by the rest of the library.

- **Serialization**: `ristretto255_decode` (32 bytes to `ge_p3`), `ristretto255_encode` (`ge_p3` to 32 bytes)
- **Comparison**: `ristretto255_equals` — constant-time equivalence check without encoding
- **Hash-to-group**: `ristretto255_from_uniform_bytes` — maps 64 uniform bytes (e.g. SHA-512 output) to a ristretto255 point via Elligator 2

### Runtime Dispatch (`ed25519_*`)

On x86_64, SIMD-accelerated backends are selected at runtime based on detected CPU features.

- **`ed25519_init(bool autotune = false)`** — Initializes the dispatch table. Without `autotune`, uses CPUID heuristics to select a good backend (~microseconds). With `autotune=true`, benchmarks all candidates and selects the empirically fastest per function (~1-2 seconds). Thread-safe; only the first call executes.
- **`ed25519_has_avx2()`**, **`ed25519_has_avx512ifma()`** — Query detected CPU features.
- **`ed25519_get_dispatch()`** — Returns a const reference to the dispatch table (read-only; only `ed25519_init` can modify it).

### Secure Erasure

- **`ed25519_secure_erase`** — Zeroes memory in a way the compiler can't optimize away. Uses `SecureZeroMemory` (MSVC/Windows), `memset_s` (C11), `explicit_bzero` (glibc 2.25+, OpenBSD, FreeBSD), or a volatile function pointer to `memset` as a last resort.

## Testing

Build with `-DBUILD_TESTS=ON` to get the `ed25519-tests` executable. CLI options:

| Flag | Effect |
|------|--------|
| *(none)* | Run with baseline dispatch (x64 or portable, no SIMD) |
| `--init` | Use CPUID heuristic dispatch before running tests |
| `--autotune` | Use benchmarked best-per-function dispatch before running tests |

The test suite has three layers:

### Hand-Written Tests

Deterministic tests for operations not covered by the other layers: `sc_reduce` (32/64-byte), `sc_clamp`, `sc_check_reduced`, `sc_check_clamped`, `sc_isnonzero`, `fe_frombytes`/`fe_tobytes` roundtrip, `ge_scalarmult_base_ct` (RFC 8032 vectors), `ge_frombytes` (roundtrip, non-canonical rejection, off-curve rejection), `ge_fromfe_frombytes`, subgroup checking, ristretto255 RFC 9496 vectors (16 generator multiples, 29 bad encodings, 7 hash-to-group, 4 equivalence), batch scalar multiplication (N=0..17), multi-scalar multiplication (Straus and Pippenger), dispatch init/autotune, and edge cases (zero scalar, identity point, scalar=1, batch-vs-single cross-validation).

### Generated Test Vectors

Independently validated deterministic vectors covering scalar, field element, group element, and ristretto255 operations. The pipeline:

1. **Generator** (`tools/gen_test_vectors.cpp`) — built with `-DFORCE_PORTABLE=ON` to use the trusted portable backend. Emits JSON to `test_vectors/ed25519_test_vectors.json`.
2. **Validator** (`tools/validate_test_vectors.py`) — independently recomputes every result using PyNaCl (libsodium bindings). Validates all vectors before they're compiled into tests.
3. **Converter** (`tools/json_to_header.py`) — converts JSON to `include/ed25519_test_vectors.h` with constexpr C++ structs.

Operations covered: `sc_add`, `sc_sub`, `sc_mul`, `sc_muladd`, `sc_mulsub`, `sc_reduce` (32/64-byte), `sc_clamp`, `fe_frombytes`/`fe_tobytes` roundtrip, `fe_add`, `fe_sub`, `fe_mul`, `fe_sq`, `fe_sq2`, `fe_neg`, `fe_invert`, `fe_pow22523`, generator/identity points, `ge_scalarmult_base_ct`, `ge_scalarmult_ct`, `ge_double_scalarmult_base_negate_vartime`, `ge_multiscalar_mul_vartime`, `ge_frombytes_vartime` (valid/invalid), `ge_p3_to_wei25519`, `ristretto255_encode`/`decode` roundtrip, `ristretto255_from_uniform_bytes`.

### Fuzz Tests (Property-Based)

25 randomized test functions exercising algebraic invariants with a fixed-seed PRNG (`std::mt19937_64`, seed=42, 256 iterations each):

- **Scalar**: commutativity, associativity, identity, inverse for add/mul; definition checks for muladd/mulsub; reduce idempotency; clamp bit properties
- **Field element**: frombytes/tobytes roundtrip idempotency; add/sub inverse; mul commutativity/associativity/distributivity; sq(f)==mul(f,f); f\*invert(f)==1; cmov correctness
- **Group element**: tobytes/frombytes roundtrip; 0\*P==identity, 1\*P==P, l\*P==identity; scalarmult linearity and compatibility; DSM matches serial
- **Batch**: batch CT/DSM/shared-scalar results match serial for various N
- **MSM**: MSM n=1 matches scalarmult, n=2..8 matches sum of individual; zero scalar, duplicate points, MSM base consistency
- **Ristretto255**: encode/decode roundtrip and determinism; equals reflexivity/symmetry; from_uniform_bytes determinism
- **Cross-backend**: captures a fingerprint of deterministic operation results before `ed25519_init()`, compares after SIMD dispatch to ensure all backends produce identical output

### Regenerating Test Vectors

```bash
# Build the generator with the portable backend
cmake -S . -B build-portable -DFORCE_PORTABLE=ON -DBUILD_TOOLS=ON
cmake --build build-portable --config Release --target ed25519-gen-testvectors

# Generate JSON
./build-portable/ed25519-gen-testvectors > test_vectors/ed25519_test_vectors.json

# Validate independently (requires PyNaCl: pip install pynacl)
python tools/validate_test_vectors.py test_vectors/ed25519_test_vectors.json

# Convert to C++ header
python tools/json_to_header.py test_vectors/ed25519_test_vectors.json include/ed25519_test_vectors.h
```

## Benchmarking

Build with `-DBUILD_BENCHMARKS=ON` to get the `ed25519-benchmark` executable. CLI options:

| Flag | Effect |
|------|--------|
| *(none)* | Baseline dispatch (x64/portable), core operations only |
| `--init` | Use CPUID heuristic dispatch |
| `--autotune` | Use benchmarked best-per-function dispatch |
| `--batch` | Include batch scalar multiplication benchmarks (N=4,8,16,64) |
| `--msm` | Include multi-scalar multiplication benchmarks (N=2,4,...,256) with speedup vs serial |

Flags can be combined: `ed25519-benchmark --init --batch --msm`

## CI

GitHub Actions runs on every push, pull request, and on a weekly schedule. Every compiler is tested across four build configurations to ensure correctness at every layer:

| Configuration | CMake flags | What it tests |
|---------------|-------------|---------------|
| **portable** | `-DFORCE_PORTABLE=ON` | 32-bit `int32_t[10]` field arithmetic on all platforms |
| **x64-baseline** | `-DENABLE_AVX2=OFF -DENABLE_AVX512=OFF` | 64-bit radix-2⁵¹ backend with no SIMD dispatch |
| **x64-avx2** | `-DENABLE_AVX512=OFF` | AVX2 SIMD backend (runtime-dispatched if CPU supports it) |
| **x64-full** | *(defaults)* | Full build with AVX2 + AVX-512 IFMA backends |

Compilers under test:

| Platform | Compilers | Configs |
|----------|-----------|---------|
| Linux (x86_64) | GCC 11, GCC 12, Clang 14, Clang 15 | All four |
| macOS (ARM64) | AppleClang, LLVM Clang | portable, arm64 |
| Windows (x86_64) | MSVC, MinGW-GCC | All four |

Unit tests and benchmarks run for every combination.

## License

This project is released into the public domain under the [Unlicense](LICENSE).
