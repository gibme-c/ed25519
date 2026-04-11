// libFuzzer harness: ge_multiscalar_mul_vartime.
//
// Splits input into N ∈ [1, MAX_N] (scalar, point) pairs, computes the MSM,
// and cross-checks against a naive ge_scalarmult_ct + ge_add reference.
// Points that fail ge_frombytes_vartime cause the iteration to be skipped.

#include "ed25519.h"
#include "ge.h"
#include "ge_add.h"
#include "ge_frombytes_vartime.h"
#include "ge_multiscalar_mul_vartime.h"
#include "ge_p1p1_to_p3.h"
#include "ge_p3_0.h"
#include "ge_p3_to_cached.h"
#include "ge_p3_tobytes.h"
#include "ge_scalarmult_ct.h"
#include "sc_reduce.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

static constexpr size_t MAX_N = 8;
static constexpr size_t PAIR_SIZE = 64; // 32 scalar + 32 point encoding

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < PAIR_SIZE + 1)
    {
        return 0;
    }

    size_t n = static_cast<size_t>(data[0] & 0x07) + 1;
    const uint8_t *p = data + 1;
    size_t avail = size - 1;
    if (avail < n * PAIR_SIZE)
    {
        n = avail / PAIR_SIZE;
        if (n == 0)
        {
            return 0;
        }
    }

    unsigned char scalars[MAX_N * 32];
    ge_p3 points[MAX_N];

    for (size_t i = 0; i < n; i++)
    {
        unsigned char sbuf[64] = {0};
        std::memcpy(sbuf, p + i * PAIR_SIZE, 32);
        sc_reduce(sbuf, 64);
        std::memcpy(&scalars[i * 32], sbuf, 32);

        unsigned char pbuf[32];
        std::memcpy(pbuf, p + i * PAIR_SIZE + 32, 32);
        if (ge_frombytes_vartime(&points[i], pbuf) != 0)
        {
            return 0;
        }
    }

    ge_p3 msm_result;
    ge_multiscalar_mul_vartime(&msm_result, scalars, points, n);

    unsigned char msm_bytes[32];
    ge_p3_tobytes(msm_bytes, &msm_result);

    ge_p3 ref;
    ge_p3_0(&ref);
    for (size_t i = 0; i < n; i++)
    {
        ge_p1p1 t;
        ge_scalarmult_ct(&t, &scalars[i * 32], &points[i]);
        ge_p3 term;
        ge_p1p1_to_p3(&term, &t);

        ge_cached cached;
        ge_p3_to_cached(&cached, &term);
        ge_p1p1 sum;
        ge_add(&sum, &ref, &cached);
        ge_p1p1_to_p3(&ref, &sum);
    }

    unsigned char ref_bytes[32];
    ge_p3_tobytes(ref_bytes, &ref);

    if (std::memcmp(msm_bytes, ref_bytes, 32) != 0)
    {
        __builtin_trap();
    }

    return 0;
}
