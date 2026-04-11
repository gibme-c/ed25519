// Empirical constant-time harness for ge_scalarmult_ct using dudect.
// Opt-in build (ED25519_BUILD_CT_TESTS=ON). Local use only — CI runners are too
// noisy for a Welch t-test to produce a reliable signal.

extern "C"
{
#define DUDECT_IMPLEMENTATION
#include "dudect.h"
}

#include "ed25519.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{

    // Fixed public point A: derived at startup via scalarmult_base_ct on a
    // constant scalar so the harness is self-contained and reproducible.
    ge_p3 g_fixed_A;

    void init_fixed_point()
    {
        unsigned char scalar[32] = {0};
        scalar[0] = 42;
        sc_clamp(scalar);
        ge_p1p1 t;
        ge_scalarmult_base_ct(&t, scalar);
        ge_p1p1_to_p3(&g_fixed_A, &t);
    }

} // namespace

extern "C" void prepare_inputs(dudect_config_t *c, uint8_t *input_data, uint8_t *classes)
{
    randombytes(input_data, c->number_measurements * c->chunk_size);
    for (size_t i = 0; i < c->number_measurements; i++)
    {
        classes[i] = randombit();
        if (classes[i] == 0)
        {
            // Class 0: all-zero scalar (except low bit set to avoid identity noise)
            std::memset(input_data + i * c->chunk_size, 0, c->chunk_size);
        }
        // Class 1: random scalar (already randomized above).

        // Clamp every scalar to satisfy the a[31] <= 127 precondition of
        // ge_scalarmult_ct, regardless of class.
        sc_clamp(input_data + i * c->chunk_size);
    }
}

extern "C" uint8_t do_one_computation(uint8_t *data)
{
    ge_p1p1 t;
    ge_scalarmult_ct(&t, data, &g_fixed_A);
    // Force a dependency on the output so the compiler cannot elide the call.
    volatile uint8_t sink = 0;
    sink ^= reinterpret_cast<const uint8_t *>(&t)[0];
    return sink;
}

int main()
{
    ed25519_init();
    init_fixed_point();

    dudect_config_t cfg {};
    cfg.chunk_size = 32;
    cfg.number_measurements = 10000;

    dudect_ctx_t ctx {};
    if (dudect_init(&ctx, &cfg) != 0)
    {
        std::fprintf(stderr, "dudect_init failed\n");
        return 1;
    }

    std::printf("running dudect on ge_scalarmult_ct (Ctrl-C to stop)\n");
    for (;;)
    {
        dudect_state_t st = dudect_main(&ctx);
        if (st == DUDECT_LEAKAGE_FOUND)
        {
            std::printf("LEAKAGE detected\n");
            // Keep running so the user can observe the progression; local tool.
        }
    }

    dudect_free(&ctx);
    return 0;
}
