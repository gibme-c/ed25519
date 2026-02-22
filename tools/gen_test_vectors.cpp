/**
This is free and unencumbered software released into the public domain.

Deterministic test vector generator for the Ed25519 library.
Emits JSON to stdout. Must be built with -DFORCE_PORTABLE=ON
to use the trusted portable backend as reference.

Usage: ed25519-gen-testvectors > test_vectors/ed25519_test_vectors.json
*/

#include "ed25519.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// ============================================================
// Portable backend enforcement (runtime)
// ============================================================
// This generator MUST be built with -DFORCE_PORTABLE=ON so that it uses
// the original trusted portable backend. The SageMath validator independently
// confirms these results, and then all other backends must match them.
//
// We check at runtime rather than compile time so the build system
// doesn't break — the executable simply refuses to run.

static void enforce_portable_backend()
{
#if !ED25519_FORCE_PORTABLE
    fprintf(stderr, "ERROR: gen_test_vectors MUST be built with -DFORCE_PORTABLE=ON\n");
    fprintf(stderr, "       to use the trusted portable backend for reference vectors.\n");
    fprintf(stderr, "       Rebuild with: cmake -DFORCE_PORTABLE=ON -DBUILD_TOOLS=ON\n");
    exit(1);
#endif

#if defined(ED25519_PLATFORM_64BIT) && ED25519_PLATFORM_64BIT
    fprintf(stderr, "ERROR: gen_test_vectors is using the 64-bit backend.\n");
    fprintf(stderr, "       FORCE_PORTABLE is not active. Refusing to generate vectors.\n");
    exit(1);
#endif

#if defined(ED25519_SIMD) && ED25519_SIMD
    fprintf(stderr, "ERROR: gen_test_vectors has SIMD dispatch enabled.\n");
    fprintf(stderr, "       Refusing to generate vectors — must use portable backend.\n");
    exit(1);
#endif
}

// ============================================================
// JSON output helpers
// ============================================================

static void print_hex(const unsigned char *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        printf("%02x", data[i]);
}

static void json_hex_field(const char *name, const unsigned char *data, size_t len, bool last = false)
{
    printf("      \"%s\": \"", name);
    print_hex(data, len);
    printf("\"%s\n", last ? "" : ",");
}



// ============================================================
// Scalar helpers
// ============================================================

// Group order l = 2^252 + 27742317777372353535851937790883648493
static const unsigned char group_order_l[32] = {
    0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7,
    0xa2, 0xde, 0xf9, 0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10};

static const unsigned char scalar_zero[32] = {0};

static const unsigned char scalar_one[32] = {1};

// l - 1
static const unsigned char scalar_l_minus_1[32] = {
    0xec, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7,
    0xa2, 0xde, 0xf9, 0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10};

// Deterministic test scalars (reduced mod l)
static const unsigned char scalar_a[32] = {
    0x31, 0x3b, 0x3f, 0x84, 0x28, 0x2b, 0xa0, 0x02, 0xb9, 0x4f, 0x8f,
    0x4c, 0xf4, 0x39, 0x24, 0xf6, 0xf5, 0x27, 0xd2, 0xf2, 0xdf, 0x36,
    0x96, 0x36, 0x11, 0x09, 0x56, 0xa8, 0xa8, 0x5d, 0x98, 0x04};

static const unsigned char scalar_b[32] = {
    0x18, 0x3c, 0x91, 0x5a, 0x04, 0xd2, 0xbc, 0x1b, 0x03, 0xf4, 0xa7,
    0xd9, 0xb1, 0x16, 0x4f, 0xe8, 0xc3, 0xb0, 0x2c, 0x0e, 0x6e, 0x1a,
    0x2b, 0xa4, 0x3f, 0xc8, 0x47, 0x8d, 0xd0, 0xa9, 0xe3, 0x02};

static const unsigned char scalar_c[32] = {
    0x04, 0x7f, 0xed, 0x51, 0x9e, 0x8a, 0xc6, 0x43, 0x0b, 0x1d, 0x5e,
    0xbf, 0x74, 0xcc, 0x3e, 0xdb, 0xaa, 0x94, 0xfe, 0x80, 0xde, 0x47,
    0x2e, 0xed, 0xc9, 0x0d, 0x1a, 0x12, 0x5c, 0xf5, 0x99, 0x06};

// ============================================================
// Field element helpers
// ============================================================

static const unsigned char fe_val_a[32] = {
    0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab,
    0x90, 0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab, 0x90, 0x78, 0x56,
    0x34, 0x12, 0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12};

static const unsigned char fe_val_b[32] = {
    0x21, 0x43, 0x65, 0x87, 0x09, 0xba, 0xdc, 0xfe, 0x21, 0x43, 0x65,
    0x87, 0x09, 0xba, 0xdc, 0xfe, 0x21, 0x43, 0x65, 0x87, 0x09, 0xba,
    0xdc, 0xfe, 0x21, 0x43, 0x65, 0x87, 0x09, 0xba, 0xdc, 0x7e};

// p - 1 = 2^255 - 20
static const unsigned char fe_p_minus_1[32] = {
    0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};

// ============================================================
// Emit scalar test vectors
// ============================================================

static void emit_scalar_vectors()
{
    printf("  \"scalars\": {\n");

    // --- sc_add ---
    {
        printf("    \"add\": [\n");

        auto emit_add = [](const char *label, const unsigned char *a, const unsigned char *b, bool last)
        {
            unsigned char out[32];
            sc_add(out, a, b);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"a\": \"");
            print_hex(a, 32);
            printf("\",\n");
            printf("        \"b\": \"");
            print_hex(b, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        emit_add("a+b", scalar_a, scalar_b, false);
        emit_add("a+0", scalar_a, scalar_zero, false);
        emit_add("0+a", scalar_zero, scalar_a, false);
        // a + (l-1-a) should give l-1
        {
            unsigned char neg_a[32];
            sc_sub(neg_a, scalar_l_minus_1, scalar_a);
            // a + neg_a == l-1
            unsigned char out[32];
            sc_add(out, scalar_a, neg_a);
            printf("      {\n");
            printf("        \"label\": \"a+(l-1-a)=l-1\",\n");
            printf("        \"a\": \"");
            print_hex(scalar_a, 32);
            printf("\",\n");
            printf("        \"b\": \"");
            print_hex(neg_a, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      },\n");
        }
        emit_add("(l-1)+1", scalar_l_minus_1, scalar_one, false);
        emit_add("(l-1)+(l-1)", scalar_l_minus_1, scalar_l_minus_1, true);
        printf("    ],\n");
    }

    // --- sc_sub ---
    {
        printf("    \"sub\": [\n");
        auto emit_sub = [](const char *label, const unsigned char *a, const unsigned char *b, bool last)
        {
            unsigned char out[32];
            sc_sub(out, a, b);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"a\": \"");
            print_hex(a, 32);
            printf("\",\n");
            printf("        \"b\": \"");
            print_hex(b, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        emit_sub("a-b", scalar_a, scalar_b, false);
        emit_sub("a-0", scalar_a, scalar_zero, false);
        emit_sub("0-a", scalar_zero, scalar_a, false);
        emit_sub("a-a", scalar_a, scalar_a, false);
        emit_sub("0-1", scalar_zero, scalar_one, false);
        emit_sub("1-(l-1)", scalar_one, scalar_l_minus_1, true);
        printf("    ],\n");
    }

    // --- sc_mul ---
    {
        printf("    \"mul\": [\n");
        auto emit_mul = [](const char *label, const unsigned char *a, const unsigned char *b, bool last)
        {
            unsigned char out[32];
            sc_mul(out, a, b);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"a\": \"");
            print_hex(a, 32);
            printf("\",\n");
            printf("        \"b\": \"");
            print_hex(b, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        emit_mul("a*b", scalar_a, scalar_b, false);
        emit_mul("a*1", scalar_a, scalar_one, false);
        emit_mul("a*0", scalar_a, scalar_zero, false);
        emit_mul("1*1", scalar_one, scalar_one, false);
        emit_mul("(l-1)*(l-1)", scalar_l_minus_1, scalar_l_minus_1, true);
        printf("    ],\n");
    }

    // --- sc_muladd ---
    {
        printf("    \"muladd\": [\n");
        auto emit_muladd = [](const char *label, const unsigned char *a, const unsigned char *b,
                              const unsigned char *c, bool last)
        {
            unsigned char out[32];
            sc_muladd(out, a, b, c);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"a\": \"");
            print_hex(a, 32);
            printf("\",\n");
            printf("        \"b\": \"");
            print_hex(b, 32);
            printf("\",\n");
            printf("        \"c\": \"");
            print_hex(c, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        emit_muladd("a*b+c", scalar_a, scalar_b, scalar_c, false);
        emit_muladd("a*1+0", scalar_a, scalar_one, scalar_zero, false);
        emit_muladd("0*b+c", scalar_zero, scalar_b, scalar_c, false);
        emit_muladd("1*1+1", scalar_one, scalar_one, scalar_one, true);
        printf("    ],\n");
    }

    // --- sc_mulsub ---
    {
        printf("    \"mulsub\": [\n");
        auto emit_mulsub = [](const char *label, const unsigned char *a, const unsigned char *b,
                              const unsigned char *c, bool last)
        {
            unsigned char out[32];
            sc_mulsub(out, a, b, c);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"a\": \"");
            print_hex(a, 32);
            printf("\",\n");
            printf("        \"b\": \"");
            print_hex(b, 32);
            printf("\",\n");
            printf("        \"c\": \"");
            print_hex(c, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        emit_mulsub("c-a*b", scalar_a, scalar_b, scalar_c, false);
        emit_mulsub("0-a*1 (negate a)", scalar_a, scalar_one, scalar_zero, false);
        emit_mulsub("c-0*b", scalar_zero, scalar_b, scalar_c, true);
        printf("    ],\n");
    }

    // --- sc_reduce (32-byte) ---
    {
        printf("    \"reduce32\": [\n");

        // l itself
        {
            unsigned char buf[32];
            memcpy(buf, group_order_l, 32);
            printf("      {\n");
            printf("        \"label\": \"reduce(l)\",\n");
            printf("        \"input\": \"");
            print_hex(group_order_l, 32);
            printf("\",\n");
            sc_reduce(buf, 32);
            printf("        \"result\": \"");
            print_hex(buf, 32);
            printf("\"\n");
            printf("      },\n");
        }

        // l + 1
        {
            unsigned char l_plus_1[32];
            memcpy(l_plus_1, group_order_l, 32);
            // Add 1 to first byte (l[0] = 0xed, so 0xee, no carry)
            l_plus_1[0] += 1;
            unsigned char buf[32];
            memcpy(buf, l_plus_1, 32);
            printf("      {\n");
            printf("        \"label\": \"reduce(l+1)\",\n");
            printf("        \"input\": \"");
            print_hex(l_plus_1, 32);
            printf("\",\n");
            sc_reduce(buf, 32);
            printf("        \"result\": \"");
            print_hex(buf, 32);
            printf("\"\n");
            printf("      },\n");
        }

        // 0xFF...FF with top bit cleared (2^255 - 1)
        {
            unsigned char maxval[32];
            memset(maxval, 0xff, 32);
            maxval[31] = 0x7f;
            unsigned char buf[32];
            memcpy(buf, maxval, 32);
            printf("      {\n");
            printf("        \"label\": \"reduce(2^255-1)\",\n");
            printf("        \"input\": \"");
            print_hex(maxval, 32);
            printf("\",\n");
            sc_reduce(buf, 32);
            printf("        \"result\": \"");
            print_hex(buf, 32);
            printf("\"\n");
            printf("      },\n");
        }

        // Zero stays zero
        {
            unsigned char buf[32] = {0};
            printf("      {\n");
            printf("        \"label\": \"reduce(0)\",\n");
            printf("        \"input\": \"");
            print_hex(buf, 32);
            printf("\",\n");
            sc_reduce(buf, 32);
            printf("        \"result\": \"");
            print_hex(buf, 32);
            printf("\"\n");
            printf("      }\n");
        }

        printf("    ],\n");
    }

    // --- sc_reduce (64-byte) ---
    {
        printf("    \"reduce64\": [\n");

        // All 0xFF (64 bytes)
        {
            unsigned char buf[64];
            memset(buf, 0xff, 64);
            buf[63] = 0x7f; // Clear top bit to keep under 2^511
            unsigned char inp[64];
            memcpy(inp, buf, 64);
            printf("      {\n");
            printf("        \"label\": \"reduce64(2^511-1)\",\n");
            printf("        \"input\": \"");
            print_hex(inp, 64);
            printf("\",\n");
            sc_reduce(buf, 64);
            printf("        \"result\": \"");
            print_hex(buf, 32);
            printf("\"\n");
            printf("      },\n");
        }

        // Deterministic 64-byte value
        {
            unsigned char buf[64];
            for (int i = 0; i < 64; i++)
                buf[i] = static_cast<unsigned char>((i * 37 + 11) & 0xFF);
            unsigned char inp[64];
            memcpy(inp, buf, 64);
            printf("      {\n");
            printf("        \"label\": \"reduce64(det1)\",\n");
            printf("        \"input\": \"");
            print_hex(inp, 64);
            printf("\",\n");
            sc_reduce(buf, 64);
            printf("        \"result\": \"");
            print_hex(buf, 32);
            printf("\"\n");
            printf("      }\n");
        }

        printf("    ],\n");
    }

    // --- sc_clamp ---
    {
        printf("    \"clamp\": [\n");
        auto emit_clamp = [](const char *label, const unsigned char *input, bool last)
        {
            unsigned char buf[32];
            memcpy(buf, input, 32);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"input\": \"");
            print_hex(input, 32);
            printf("\",\n");
            sc_clamp(buf);
            printf("        \"result\": \"");
            print_hex(buf, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        emit_clamp("clamp(a)", scalar_a, false);
        emit_clamp("clamp(b)", scalar_b, false);

        // All zeros
        unsigned char zeros[32] = {0};
        emit_clamp("clamp(0)", zeros, false);

        // All 0xFF
        unsigned char allff[32];
        memset(allff, 0xff, 32);
        emit_clamp("clamp(0xFF...FF)", allff, true);

        printf("    ],\n");
    }

    // --- sc negate: 0 - a mod l ---
    {
        printf("    \"negate\": [\n");
        auto emit_neg = [](const char *label, const unsigned char *a, bool last)
        {
            unsigned char out[32];
            sc_sub(out, scalar_zero, a);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"input\": \"");
            print_hex(a, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        emit_neg("neg(0)", scalar_zero, false);
        emit_neg("neg(1)", scalar_one, false);
        emit_neg("neg(a)", scalar_a, false);
        emit_neg("neg(l-1)", scalar_l_minus_1, true);
        printf("    ]\n");
    }

    printf("  },\n");
}

// ============================================================
// Emit field element test vectors
// ============================================================

static void emit_fe_vectors()
{
    printf("  \"field_elements\": {\n");

    // --- frombytes/tobytes roundtrip ---
    {
        printf("    \"roundtrip\": [\n");
        auto emit_rt = [](const char *label, const unsigned char *input, bool last)
        {
            fe f;
            fe_frombytes(f, input);
            unsigned char out[32];
            fe_tobytes(out, f);
            printf("      {\n");
            printf("        \"label\": \"%s\",\n", label);
            printf("        \"input\": \"");
            print_hex(input, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", last ? "" : ",");
        };

        // 42
        unsigned char v42[32] = {42};
        emit_rt("42", v42, false);

        // p - 1 = 2^255 - 20
        emit_rt("p-1", fe_p_minus_1, false);

        // p (should reduce to 0)
        unsigned char p[32] = {0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};
        emit_rt("p (reduces to 0)", p, false);

        // p + 1
        unsigned char p1[32];
        memcpy(p1, p, 32);
        p1[0] += 1;
        emit_rt("p+1 (reduces to 1)", p1, false);

        // 2^255 - 1
        unsigned char maxval[32];
        memset(maxval, 0xff, 32);
        maxval[31] = 0x7f;
        emit_rt("2^255-1", maxval, false);

        // Zero
        unsigned char zero[32] = {0};
        emit_rt("0", zero, false);

        // One
        unsigned char one[32] = {1};
        emit_rt("1", one, false);

        // Random-ish
        emit_rt("fe_a", fe_val_a, false);
        emit_rt("fe_b", fe_val_b, true);

        printf("    ],\n");
    }

    // --- fe arithmetic ---
    {
        fe a, b;
        fe_frombytes(a, fe_val_a);
        fe_frombytes(b, fe_val_b);
        unsigned char out[32];

        // add
        {
            fe r;
            fe_add(r, a, b);
            fe_tobytes(out, r);
            printf("    \"add\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("b", fe_val_b, 32);
            json_hex_field("result", out, 32, true);
            printf("    },\n");
        }

        // sub
        {
            fe r;
            fe_sub(r, a, b);
            fe_tobytes(out, r);
            printf("    \"sub\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("b", fe_val_b, 32);
            json_hex_field("result", out, 32, true);
            printf("    },\n");
        }

        // mul
        {
            fe r;
            fe_mul(r, a, b);
            fe_tobytes(out, r);
            printf("    \"mul\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("b", fe_val_b, 32);
            json_hex_field("result", out, 32, true);
            printf("    },\n");
        }

        // sq
        {
            fe r;
            fe_sq(r, a);
            fe_tobytes(out, r);
            printf("    \"sq\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("result", out, 32, true);
            printf("    },\n");
        }

        // sq2
        {
            fe r;
            fe_sq2(r, a);
            fe_tobytes(out, r);
            printf("    \"sq2\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("result", out, 32, true);
            printf("    },\n");
        }

        // neg
        {
            fe r;
            fe_neg(r, a);
            fe_tobytes(out, r);
            printf("    \"neg\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("result", out, 32, true);
            printf("    },\n");
        }

        // invert
        {
            fe r;
            fe_invert(r, a);
            fe_tobytes(out, r);
            printf("    \"invert\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("result", out, 32, true);
            printf("    },\n");
        }

        // pow22523
        {
            fe r;
            fe_pow22523(r, a);
            fe_tobytes(out, r);
            printf("    \"pow22523\": {\n");
            json_hex_field("a", fe_val_a, 32);
            json_hex_field("result", out, 32, true);
            printf("    }\n");
        }
    }

    printf("  },\n");
}

// ============================================================
// Emit group element test vectors
// ============================================================

static void emit_ge_vectors()
{
    printf("  \"group_elements\": {\n");

    // --- Base point (generator) ---
    {
        ge_p1p1 r;
        ge_p3 p3;
        unsigned char out[32];
        ge_scalarmult_base_ct(&r, scalar_one);
        ge_p1p1_to_p3(&p3, &r);
        ge_p3_tobytes(out, &p3);
        printf("    \"generator\": \"");
        print_hex(out, 32);
        printf("\",\n");
    }

    // --- Identity ---
    {
        ge_p3 id;
        ge_p3_0(&id);
        unsigned char out[32];
        ge_p3_tobytes(out, &id);
        printf("    \"identity\": \"");
        print_hex(out, 32);
        printf("\",\n");
    }

    // --- scalar_mul: various scalars × base point ---
    {
        printf("    \"scalar_mul_base\": [\n");

        const struct
        {
            const char *label;
            const unsigned char *scalar;
        } sm_tests[] = {
            {"0*B", scalar_zero},
            {"1*B", scalar_one},
            {"a*B", scalar_a},
            {"b*B", scalar_b},
            {"(l-1)*B", scalar_l_minus_1},
        };

        size_t n = sizeof(sm_tests) / sizeof(sm_tests[0]);
        for (size_t i = 0; i < n; i++)
        {
            ge_p1p1 r;
            ge_p3 p3;
            unsigned char out[32];

            ge_scalarmult_base_ct(&r, sm_tests[i].scalar);
            ge_p1p1_to_p3(&p3, &r);
            ge_p3_tobytes(out, &p3);

            printf("      {\n");
            printf("        \"label\": \"%s\",\n", sm_tests[i].label);
            printf("        \"scalar\": \"");
            print_hex(sm_tests[i].scalar, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", (i == n - 1) ? "" : ",");
        }
        printf("    ],\n");
    }

    // --- scalar_mul: scalar × arbitrary point ---
    {
        printf("    \"scalar_mul_varbase\": [\n");

        // Generate a test point: scalar_c * B
        ge_p1p1 tmp;
        ge_p3 P;
        ge_scalarmult_base_ct(&tmp, scalar_c);
        ge_p1p1_to_p3(&P, &tmp);
        unsigned char P_bytes[32];
        ge_p3_tobytes(P_bytes, &P);

        const struct
        {
            const char *label;
            const unsigned char *scalar;
        } tests[] = {
            {"0*P", scalar_zero},
            {"1*P", scalar_one},
            {"a*P", scalar_a},
        };

        size_t n = sizeof(tests) / sizeof(tests[0]);
        for (size_t i = 0; i < n; i++)
        {
            ge_p1p1 r;
            ge_p3 p3;
            unsigned char out[32];

            ge_scalarmult_ct(&r, tests[i].scalar, &P);
            ge_p1p1_to_p3(&p3, &r);
            ge_p3_tobytes(out, &p3);

            printf("      {\n");
            printf("        \"label\": \"%s\",\n", tests[i].label);
            printf("        \"scalar\": \"");
            print_hex(tests[i].scalar, 32);
            printf("\",\n");
            printf("        \"point\": \"");
            print_hex(P_bytes, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", (i == n - 1) ? "" : ",");
        }
        printf("    ],\n");
    }

    // --- double scalar mul: a*A + b*(-B) (verification equation) ---
    {
        printf("    \"double_scalar_mul_base\": [\n");

        // Generate point A = scalar_c * BasePoint
        ge_p1p1 tmp;
        ge_p3 A;
        ge_scalarmult_base_ct(&tmp, scalar_c);
        ge_p1p1_to_p3(&A, &tmp);
        unsigned char A_bytes[32];
        ge_p3_tobytes(A_bytes, &A);

        const struct
        {
            const char *label;
            const unsigned char *a_sc;
            const unsigned char *b_sc;
        } tests[] = {
            {"a*A+b*B", scalar_a, scalar_b},
            {"0*A+b*B", scalar_zero, scalar_b},
            {"a*A+0*B", scalar_a, scalar_zero},
            {"1*A+1*B", scalar_one, scalar_one},
        };

        size_t n = sizeof(tests) / sizeof(tests[0]);
        for (size_t i = 0; i < n; i++)
        {
            ge_p1p1 r;
            ge_double_scalarmult_base_negate_vartime(&r, tests[i].a_sc, &A, tests[i].b_sc);
            ge_p2 p2;
            ge_p1p1_to_p2(&p2, &r);
            unsigned char out[32];
            ge_tobytes(out, &p2);

            printf("      {\n");
            printf("        \"label\": \"%s\",\n", tests[i].label);
            printf("        \"a\": \"");
            print_hex(tests[i].a_sc, 32);
            printf("\",\n");
            printf("        \"A\": \"");
            print_hex(A_bytes, 32);
            printf("\",\n");
            printf("        \"b\": \"");
            print_hex(tests[i].b_sc, 32);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", (i == n - 1) ? "" : ",");
        }
        printf("    ],\n");
    }

    // --- MSM ---
    {
        printf("    \"msm\": [\n");
        size_t msm_counts[] = {1, 2, 4, 8};

        for (size_t ci = 0; ci < sizeof(msm_counts) / sizeof(msm_counts[0]); ci++)
        {
            size_t count = msm_counts[ci];

            // Generate deterministic scalars and points
            std::vector<unsigned char> scalars(count * 32);
            std::vector<ge_p3> points(count);

            for (size_t i = 0; i < count; i++)
            {
                // Deterministic scalar
                unsigned char seed_scalar[32];
                for (int j = 0; j < 32; j++)
                    seed_scalar[j] = static_cast<unsigned char>((i * 53 + j * 17 + 7) & 0xFF);
                seed_scalar[31] &= 0x7F;
                sc_reduce(seed_scalar, 32);
                memcpy(scalars.data() + i * 32, seed_scalar, 32);

                // Deterministic point: scalarmult_base(seed)
                unsigned char point_seed[32];
                for (int j = 0; j < 32; j++)
                    point_seed[j] = static_cast<unsigned char>((i * 71 + j * 31 + 13) & 0xFF);
                point_seed[0] &= 248;
                point_seed[31] &= 127;
                point_seed[31] |= 64;
                ge_p1p1 tmp;
                ge_scalarmult_base_ct(&tmp, point_seed);
                ge_p1p1_to_p3(&points[i], &tmp);
            }

            ge_p3 result;
            ge_multiscalar_mul_vartime(&result, scalars.data(), points.data(), count);
            unsigned char out[32];
            ge_p3_tobytes(out, &result);

            printf("      {\n");
            printf("        \"n\": %zu,\n", count);
            printf("        \"scalars\": [");
            for (size_t i = 0; i < count; i++)
            {
                printf("\"");
                print_hex(scalars.data() + i * 32, 32);
                printf("\"%s", (i == count - 1) ? "" : ", ");
            }
            printf("],\n");
            printf("        \"points\": [");
            for (size_t i = 0; i < count; i++)
            {
                unsigned char pt_bytes[32];
                ge_p3_tobytes(pt_bytes, &points[i]);
                printf("\"");
                print_hex(pt_bytes, 32);
                printf("\"%s", (i == count - 1) ? "" : ", ");
            }
            printf("],\n");
            printf("        \"result\": \"");
            print_hex(out, 32);
            printf("\"\n");
            printf("      }%s\n", (ci == sizeof(msm_counts) / sizeof(msm_counts[0]) - 1) ? "" : ",");
        }
        printf("    ],\n");
    }

    // --- ge_frombytes: valid / invalid ---
    {
        printf("    \"frombytes\": {\n");
        printf("      \"valid\": [\n");

        // Base point
        unsigned char base[32] = {0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66};
        ge_p3 p3;
        int rc = ge_frombytes_vartime(&p3, base);
        printf("        {\"label\": \"base point\", \"input\": \"");
        print_hex(base, 32);
        printf("\", \"rc\": %d},\n", rc);

        // Identity
        unsigned char identity[32] = {0x01};
        rc = ge_frombytes_vartime(&p3, identity);
        printf("        {\"label\": \"identity\", \"input\": \"");
        print_hex(identity, 32);
        printf("\", \"rc\": %d}\n", rc);

        printf("      ],\n");

        printf("      \"invalid\": [\n");
        // Non-canonical: y = p
        unsigned char noncanon[32] = {0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};
        rc = ge_frombytes_vartime(&p3, noncanon);
        printf("        {\"label\": \"y=p non-canonical\", \"input\": \"");
        print_hex(noncanon, 32);
        printf("\", \"rc\": %d},\n", rc);

        // Off-curve: y=2, x^2 = (y^2-1)/(dy^2+1) has no sqrt
        unsigned char offcurve[32] = {2};
        rc = ge_frombytes_vartime(&p3, offcurve);
        printf("        {\"label\": \"y=2 off-curve\", \"input\": \"");
        print_hex(offcurve, 32);
        printf("\", \"rc\": %d}\n", rc);

        printf("      ]\n");
        printf("    },\n");
    }

    // --- Wei25519 X-coordinate ---
    {
        printf("    \"wei25519\": [\n");
        unsigned char scalars[][32] = {{1}, {2}, {3}, {5}, {7}, {11}};
        size_t n = sizeof(scalars) / sizeof(scalars[0]);

        for (size_t i = 0; i < n; i++)
        {
            ge_p1p1 r;
            ge_p3 pt;
            ge_scalarmult_base_ct(&r, scalars[i]);
            ge_p1p1_to_p3(&pt, &r);

            unsigned char wei[32];
            ge_p3_to_wei25519(wei, &pt);

            printf("      {\n");
            printf("        \"scalar\": \"");
            print_hex(scalars[i], 32);
            printf("\",\n");
            printf("        \"wei25519_x\": \"");
            print_hex(wei, 32);
            printf("\"\n");
            printf("      }%s\n", (i == n - 1) ? "" : ",");
        }
        printf("    ]\n");
    }

    printf("  },\n");
}

// ============================================================
// Emit ristretto255 test vectors
// ============================================================

static void emit_ristretto_vectors()
{
    printf("  \"ristretto255\": {\n");

    // --- encode/decode roundtrip on generator multiples ---
    {
        printf("    \"roundtrip\": [\n");
        ge_p3 accum;
        ge_p3_0(&accum);

        // Generator
        ge_p1p1 base_p1p1;
        ge_p3 base_p3;
        ge_scalarmult_base_ct(&base_p1p1, scalar_one);
        ge_p1p1_to_p3(&base_p3, &base_p1p1);

        for (int i = 0; i <= 8; i++)
        {
            unsigned char encoded[32];
            ristretto255_encode(encoded, &accum);

            ge_p3 decoded;
            int rc = ristretto255_decode(&decoded, encoded);

            unsigned char re_encoded[32];
            ristretto255_encode(re_encoded, &decoded);

            printf("      {\n");
            printf("        \"label\": \"%d*G\",\n", i);
            printf("        \"encoded\": \"");
            print_hex(encoded, 32);
            printf("\",\n");
            printf("        \"decode_rc\": %d,\n", rc);
            printf("        \"re_encoded\": \"");
            print_hex(re_encoded, 32);
            printf("\"\n");
            printf("      }%s\n", (i == 8) ? "" : ",");

            // accum += G
            if (i < 8)
            {
                ge_cached cached;
                ge_p3_to_cached(&cached, &base_p3);
                ge_p1p1 sum;
                ge_add(&sum, &accum, &cached);
                ge_p1p1_to_p3(&accum, &sum);
            }
        }
        printf("    ],\n");
    }

    // --- from_uniform_bytes ---
    {
        printf("    \"from_uniform_bytes\": [\n");
        for (int i = 0; i < 4; i++)
        {
            unsigned char input[64];
            for (int j = 0; j < 64; j++)
                input[j] = static_cast<unsigned char>((i * 41 + j * 17 + 3) & 0xFF);

            ge_p3 p;
            ristretto255_from_uniform_bytes(&p, input);

            unsigned char encoded[32];
            ristretto255_encode(encoded, &p);

            printf("      {\n");
            printf("        \"label\": \"hash_to_group_%d\",\n", i);
            printf("        \"input\": \"");
            print_hex(input, 64);
            printf("\",\n");
            printf("        \"result\": \"");
            print_hex(encoded, 32);
            printf("\"\n");
            printf("      }%s\n", (i == 3) ? "" : ",");
        }
        printf("    ]\n");
    }

    printf("  }\n");
}

// ============================================================
// Main
// ============================================================

int main()
{
    enforce_portable_backend();

    printf("{\n");
    printf("  \"generator\": \"ed25519-gen-testvectors (portable backend)\",\n");
    printf("  \"version\": 1,\n");

    emit_scalar_vectors();
    emit_fe_vectors();
    emit_ge_vectors();
    emit_ristretto_vectors();

    printf("}\n");

    return 0;
}
