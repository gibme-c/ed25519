// libFuzzer harness: fe_frombytes → fe_tobytes canonical round-trip.
//
// Property: fe_tobytes(fe_frombytes(x)) must produce a byte string that
// (a) has the high bit clear and (b) reproduces itself through a second
// round-trip. The second round-trip pins canonicalization — any
// non-idempotent encoding fails loudly.

#include "ed25519.h"
#include "fe.h"
#include "fe_frombytes.h"
#include "fe_tobytes.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 32)
    {
        return 0;
    }

    unsigned char in[32];
    std::memcpy(in, data, 32);

    fe f;
    fe_frombytes(f, in);

    unsigned char out1[32];
    fe_tobytes(out1, f);

    // Output MUST have high bit clear (canonical encoding).
    if (out1[31] & 0x80)
    {
        __builtin_trap();
    }

    // Round-trip idempotency: fe_frombytes/fe_tobytes on a canonical encoding
    // must yield the same bytes.
    fe g;
    fe_frombytes(g, out1);
    unsigned char out2[32];
    fe_tobytes(out2, g);
    if (std::memcmp(out1, out2, 32) != 0)
    {
        __builtin_trap();
    }

    return 0;
}
