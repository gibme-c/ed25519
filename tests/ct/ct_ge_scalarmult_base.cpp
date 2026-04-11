// Empirical constant-time harness for ge_scalarmult_base_ct using dudect.
// Opt-in build (ED25519_BUILD_CT_TESTS=ON). Local use only.

extern "C"
{
#define DUDECT_IMPLEMENTATION
#include "dudect.h"
}

#include "ed25519.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" void prepare_inputs(dudect_config_t *c, uint8_t *input_data, uint8_t *classes)
{
    randombytes(input_data, c->number_measurements * c->chunk_size);
    for (size_t i = 0; i < c->number_measurements; i++)
    {
        classes[i] = randombit();
        if (classes[i] == 0)
        {
            std::memset(input_data + i * c->chunk_size, 0, c->chunk_size);
        }
        sc_clamp(input_data + i * c->chunk_size);
    }
}

extern "C" uint8_t do_one_computation(uint8_t *data)
{
    ge_p1p1 r;
    ge_scalarmult_base_ct(&r, data);
    volatile uint8_t sink = 0;
    sink ^= reinterpret_cast<const uint8_t *>(&r)[0];
    return sink;
}

int main()
{
    ed25519_init();

    dudect_config_t cfg {};
    cfg.chunk_size = 32;
    cfg.number_measurements = 10000;

    dudect_ctx_t ctx {};
    if (dudect_init(&ctx, &cfg) != 0)
    {
        std::fprintf(stderr, "dudect_init failed\n");
        return 1;
    }

    std::printf("running dudect on ge_scalarmult_base_ct (Ctrl-C to stop)\n");
    for (;;)
    {
        dudect_state_t st = dudect_main(&ctx);
        if (st == DUDECT_LEAKAGE_FOUND)
        {
            std::printf("LEAKAGE detected\n");
        }
    }

    dudect_free(&ctx);
    return 0;
}
