// libFuzzer harness: ristretto255_decode round-trip (RFC 9496 canonicalization).
//
// On any successful decode, re-encoding must reproduce the exact input bytes.
// This pins the RFC 9496 §4.3.1 canonicalization requirements (non-canonical
// rejection, IS_NEGATIVE rejection, was_square check, etc).

#include "ed25519.h"
#include "ge.h"
#include "ristretto255.h"

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

    ge_p3 p;
    int rc = ristretto255_decode(&p, in);
    if (rc != 0)
    {
        return 0;
    }

    unsigned char out[32];
    ristretto255_encode(out, &p);
    if (std::memcmp(in, out, 32) != 0)
    {
        __builtin_trap();
    }

    return 0;
}
