// libFuzzer harness: ristretto255_from_uniform_bytes.
//
// Property: hash-to-group must always produce a point that round-trips
// encode/decode successfully. Any input that yields a point failing this
// invariant indicates a broken canonicalization path.

#include "ed25519.h"
#include "ge.h"
#include "ristretto255.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 64)
    {
        return 0;
    }

    unsigned char uniform[64];
    std::memcpy(uniform, data, 64);

    ge_p3 p;
    ristretto255_from_uniform_bytes(&p, uniform);

    unsigned char encoded[32];
    ristretto255_encode(encoded, &p);

    ge_p3 p2;
    int rc = ristretto255_decode(&p2, encoded);
    if (rc != 0)
    {
        __builtin_trap();
    }

    unsigned char encoded2[32];
    ristretto255_encode(encoded2, &p2);
    if (std::memcmp(encoded, encoded2, 32) != 0)
    {
        __builtin_trap();
    }

    return 0;
}
