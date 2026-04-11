// libFuzzer harness: sc_check_reduced.
//
// Memory safety only. The function's correctness is covered by deterministic
// unit tests with hand-picked boundary values; here we fuzz for ASan/UBSan
// coverage across the full 2^256 input space.

#include "ed25519.h"
#include "sc_check_reduced.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 32)
    {
        return 0;
    }

    unsigned char s[32];
    std::memcpy(s, data, 32);
    (void)sc_check_reduced(s);
    return 0;
}
