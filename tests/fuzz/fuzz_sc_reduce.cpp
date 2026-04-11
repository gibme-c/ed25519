// libFuzzer harness: sc_reduce.
//
// Property: sc_reduce(x) reduces a 64-byte input mod l. The output must
// always satisfy sc_check_reduced (i.e. < l).

#include "ed25519.h"
#include "sc_check_reduced.h"
#include "sc_reduce.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 64)
    {
        return 0;
    }

    unsigned char buf[64];
    std::memcpy(buf, data, 64);

    sc_reduce(buf, 64);

    // After reduction, the first 32 bytes must be < l.
    if (sc_check_reduced(buf) != 0)
    {
        __builtin_trap();
    }

    return 0;
}
