// libFuzzer harness: x25519.
//
// Memory-safety and UB coverage for the Montgomery ladder across arbitrary
// (scalar, u) inputs. Correctness against reference vectors is handled in
// the unit tests; this target is ASan/UBSan-focused.
//
// Also asserts the RFC 7748 §5 invariant: x25519(k, u) == x25519(k, u | 0x80).

#include "ed25519.h"
#include "x25519.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 64)
    {
        return 0;
    }

    unsigned char scalar[32];
    unsigned char u[32];
    std::memcpy(scalar, data, 32);
    std::memcpy(u, data + 32, 32);

    unsigned char out_clean[32];
    x25519(out_clean, scalar, u);

    unsigned char u_msb[32];
    std::memcpy(u_msb, u, 32);
    u_msb[31] |= 0x80;

    unsigned char out_msb[32];
    x25519(out_msb, scalar, u_msb);

    if (std::memcmp(out_clean, out_msb, 32) != 0)
    {
        __builtin_trap();
    }

    return 0;
}
