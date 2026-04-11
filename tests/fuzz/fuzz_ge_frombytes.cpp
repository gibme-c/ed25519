// libFuzzer harness: ge_frombytes_vartime.
//
// Memory-safety property (ASan/UBSan): ge_frombytes_vartime must not read or
// write out-of-bounds for any 32-byte input, success or failure.
// Correctness property: on success, ge_p3_tobytes(ge_frombytes_vartime(x)) == x.

#include "ed25519.h"
#include "ge.h"
#include "ge_frombytes_vartime.h"
#include "ge_p3_tobytes.h"

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
    int rc = ge_frombytes_vartime(&p, in);
    if (rc != 0)
    {
        return 0;
    }

    unsigned char out[32];
    ge_p3_tobytes(out, &p);
    if (std::memcmp(in, out, 32) != 0)
    {
        __builtin_trap();
    }

    return 0;
}
