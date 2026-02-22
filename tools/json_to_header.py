#!/usr/bin/env python3
"""
Converts ed25519_test_vectors.json to include/ed25519_test_vectors.h.

Usage:
    python tools/json_to_header.py test_vectors/ed25519_test_vectors.json include/ed25519_test_vectors.h
"""

import json
import sys
import textwrap


def hex_to_c_array(hex_str, indent=8):
    """Convert hex string to C byte array initializer."""
    b = bytes.fromhex(hex_str)
    parts = []
    for i in range(0, len(b), 11):
        chunk = b[i:i+11]
        parts.append(", ".join(f"0x{byte:02x}" for byte in chunk))
    sep = ",\n" + " " * indent
    return sep.join(parts)


def emit_scalar_section(out, scalars):
    """Emit scalar test vector structs."""
    out.append("    // =========================================")
    out.append("    // Scalar operations")
    out.append("    // =========================================")
    out.append("")

    # sc_add
    add_vecs = scalars.get("add", [])
    out.append(f"    static constexpr int SC_ADD_COUNT = {len(add_vecs)};")
    out.append("    struct ScBinaryVec { const char *label; unsigned char a[32]; unsigned char b[32]; unsigned char result[32]; };")
    out.append("    static constexpr ScBinaryVec sc_add_vectors[] = {")
    for v in add_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["a"])}}},')
        out.append(f'         {{{hex_to_c_array(v["b"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_sub
    sub_vecs = scalars.get("sub", [])
    out.append(f"    static constexpr int SC_SUB_COUNT = {len(sub_vecs)};")
    out.append("    static constexpr ScBinaryVec sc_sub_vectors[] = {")
    for v in sub_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["a"])}}},')
        out.append(f'         {{{hex_to_c_array(v["b"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_mul
    mul_vecs = scalars.get("mul", [])
    out.append(f"    static constexpr int SC_MUL_COUNT = {len(mul_vecs)};")
    out.append("    static constexpr ScBinaryVec sc_mul_vectors[] = {")
    for v in mul_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["a"])}}},')
        out.append(f'         {{{hex_to_c_array(v["b"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_muladd
    muladd_vecs = scalars.get("muladd", [])
    out.append(f"    static constexpr int SC_MULADD_COUNT = {len(muladd_vecs)};")
    out.append("    struct ScTernaryVec { const char *label; unsigned char a[32]; unsigned char b[32]; unsigned char c[32]; unsigned char result[32]; };")
    out.append("    static constexpr ScTernaryVec sc_muladd_vectors[] = {")
    for v in muladd_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["a"])}}},')
        out.append(f'         {{{hex_to_c_array(v["b"])}}},')
        out.append(f'         {{{hex_to_c_array(v["c"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_mulsub
    mulsub_vecs = scalars.get("mulsub", [])
    out.append(f"    static constexpr int SC_MULSUB_COUNT = {len(mulsub_vecs)};")
    out.append("    static constexpr ScTernaryVec sc_mulsub_vectors[] = {")
    for v in mulsub_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["a"])}}},')
        out.append(f'         {{{hex_to_c_array(v["b"])}}},')
        out.append(f'         {{{hex_to_c_array(v["c"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_reduce32
    reduce32_vecs = scalars.get("reduce32", [])
    out.append(f"    static constexpr int SC_REDUCE32_COUNT = {len(reduce32_vecs)};")
    out.append("    struct ScReduceVec { const char *label; unsigned char input[32]; unsigned char result[32]; };")
    out.append("    static constexpr ScReduceVec sc_reduce32_vectors[] = {")
    for v in reduce32_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["input"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_reduce64
    reduce64_vecs = scalars.get("reduce64", [])
    out.append(f"    static constexpr int SC_REDUCE64_COUNT = {len(reduce64_vecs)};")
    out.append("    struct ScReduce64Vec { const char *label; unsigned char input[64]; unsigned char result[32]; };")
    out.append("    static constexpr ScReduce64Vec sc_reduce64_vectors[] = {")
    for v in reduce64_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["input"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_clamp
    clamp_vecs = scalars.get("clamp", [])
    out.append(f"    static constexpr int SC_CLAMP_COUNT = {len(clamp_vecs)};")
    out.append("    static constexpr ScReduceVec sc_clamp_vectors[] = {")
    for v in clamp_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["input"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # sc_negate
    neg_vecs = scalars.get("negate", [])
    out.append(f"    static constexpr int SC_NEGATE_COUNT = {len(neg_vecs)};")
    out.append("    static constexpr ScReduceVec sc_negate_vectors[] = {")
    for v in neg_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["input"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")


def emit_fe_section(out, fe):
    """Emit field element test vector structs."""
    out.append("    // =========================================")
    out.append("    // Field element operations")
    out.append("    // =========================================")
    out.append("")

    # Roundtrip
    rt_vecs = fe.get("roundtrip", [])
    out.append(f"    static constexpr int FE_ROUNDTRIP_COUNT = {len(rt_vecs)};")
    out.append("    struct FeRoundtripVec { const char *label; unsigned char input[32]; unsigned char result[32]; };")
    out.append("    static constexpr FeRoundtripVec fe_roundtrip_vectors[] = {")
    for v in rt_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["input"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # Binary ops: add, sub, mul
    out.append("    struct FeBinaryVec { const char *label; unsigned char a[32]; unsigned char b[32]; unsigned char result[32]; };")
    for op in ["add", "sub", "mul"]:
        if op in fe:
            v = fe[op]
            out.append(f"    static constexpr FeBinaryVec fe_{op}_vector = {{")
            out.append(f'        "{op}",')
            out.append(f'        {{{hex_to_c_array(v["a"])}}},')
            out.append(f'        {{{hex_to_c_array(v["b"])}}},')
            out.append(f'        {{{hex_to_c_array(v["result"])}}}')
            out.append("    };")
            out.append("")

    # Unary ops: sq, sq2, neg, invert, pow22523
    out.append("    struct FeUnaryVec { const char *label; unsigned char a[32]; unsigned char result[32]; };")
    for op in ["sq", "sq2", "neg", "invert", "pow22523"]:
        if op in fe:
            v = fe[op]
            out.append(f"    static constexpr FeUnaryVec fe_{op}_vector = {{")
            out.append(f'        "{op}",')
            out.append(f'        {{{hex_to_c_array(v["a"])}}},')
            out.append(f'        {{{hex_to_c_array(v["result"])}}}')
            out.append("    };")
            out.append("")


def emit_ge_section(out, ge):
    """Emit group element test vector structs."""
    out.append("    // =========================================")
    out.append("    // Group element operations")
    out.append("    // =========================================")
    out.append("")

    # Generator and identity
    if "generator" in ge:
        out.append(f"    static constexpr unsigned char generator[] = {{{hex_to_c_array(ge['generator'])}}};")
    if "identity" in ge:
        out.append(f"    static constexpr unsigned char identity[] = {{{hex_to_c_array(ge['identity'])}}};")
    out.append("")

    # scalar_mul_base
    sm_vecs = ge.get("scalar_mul_base", [])
    out.append(f"    static constexpr int SM_BASE_COUNT = {len(sm_vecs)};")
    out.append("    struct SmBaseVec { const char *label; unsigned char scalar[32]; unsigned char result[32]; };")
    out.append("    static constexpr SmBaseVec sm_base_vectors[] = {")
    for v in sm_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["scalar"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # scalar_mul_varbase
    sv_vecs = ge.get("scalar_mul_varbase", [])
    out.append(f"    static constexpr int SM_VARBASE_COUNT = {len(sv_vecs)};")
    out.append("    struct SmVarbaseVec { const char *label; unsigned char scalar[32]; unsigned char point[32]; unsigned char result[32]; };")
    out.append("    static constexpr SmVarbaseVec sm_varbase_vectors[] = {")
    for v in sv_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["scalar"])}}},')
        out.append(f'         {{{hex_to_c_array(v["point"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # double_scalar_mul_base
    dsm_vecs = ge.get("double_scalar_mul_base", [])
    out.append(f"    static constexpr int DSM_BASE_COUNT = {len(dsm_vecs)};")
    out.append("    struct DsmBaseVec { const char *label; unsigned char a[32]; unsigned char A[32]; unsigned char b[32]; unsigned char result[32]; };")
    out.append("    static constexpr DsmBaseVec dsm_base_vectors[] = {")
    for v in dsm_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["a"])}}},')
        out.append(f'         {{{hex_to_c_array(v["A"])}}},')
        out.append(f'         {{{hex_to_c_array(v["b"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # MSM
    msm_vecs = ge.get("msm", [])
    out.append(f"    static constexpr int MSM_COUNT = {len(msm_vecs)};")
    # For MSM, we need variable-length arrays — use a flat structure
    out.append("    struct MsmVec { int n; unsigned char result[32]; };")
    out.append("    static constexpr MsmVec msm_vectors[] = {")
    for v in msm_vecs:
        out.append(f'        {{{v["n"]}, {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")

    # MSM scalars and points as flat arrays
    for idx, v in enumerate(msm_vecs):
        n = v["n"]
        out.append(f"    static constexpr unsigned char msm_{idx}_scalars[] = {{")
        for s in v["scalars"]:
            out.append(f"        {hex_to_c_array(s)},")
        out.append("    };")
        out.append(f"    static constexpr unsigned char msm_{idx}_points[] = {{")
        for pt in v["points"]:
            out.append(f"        {hex_to_c_array(pt)},")
        out.append("    };")
        out.append("")

    # frombytes valid/invalid
    fb = ge.get("frombytes", {})
    valid_vecs = fb.get("valid", [])
    out.append(f"    static constexpr int FB_VALID_COUNT = {len(valid_vecs)};")
    out.append("    struct FbVec { const char *label; unsigned char input[32]; int rc; };")
    out.append("    static constexpr FbVec fb_valid_vectors[] = {")
    for v in valid_vecs:
        out.append(f'        {{"{v["label"]}", {{{hex_to_c_array(v["input"])}}}, {v["rc"]}}},')
    out.append("    };")

    invalid_vecs = fb.get("invalid", [])
    out.append(f"    static constexpr int FB_INVALID_COUNT = {len(invalid_vecs)};")
    out.append("    static constexpr FbVec fb_invalid_vectors[] = {")
    for v in invalid_vecs:
        out.append(f'        {{"{v["label"]}", {{{hex_to_c_array(v["input"])}}}, {v["rc"]}}},')
    out.append("    };")
    out.append("")

    # Wei25519
    wei_vecs = ge.get("wei25519", [])
    out.append(f"    static constexpr int WEI25519_COUNT = {len(wei_vecs)};")
    out.append("    struct Wei25519Vec { unsigned char scalar[32]; unsigned char wei25519_x[32]; };")
    out.append("    static constexpr Wei25519Vec wei25519_vectors[] = {")
    for v in wei_vecs:
        out.append(f'        {{{{{hex_to_c_array(v["scalar"])}}},')
        out.append(f'         {{{hex_to_c_array(v["wei25519_x"])}}}}},')
    out.append("    };")
    out.append("")


def emit_ristretto_section(out, rist):
    """Emit ristretto255 test vector structs."""
    out.append("    // =========================================")
    out.append("    // Ristretto255")
    out.append("    // =========================================")
    out.append("")

    # Roundtrip
    rt_vecs = rist.get("roundtrip", [])
    out.append(f"    static constexpr int RISTRETTO_RT_COUNT = {len(rt_vecs)};")
    out.append("    struct RistrettoRtVec { const char *label; unsigned char encoded[32]; };")
    out.append("    static constexpr RistrettoRtVec ristretto_rt_vectors[] = {")
    for v in rt_vecs:
        out.append(f'        {{"{v["label"]}", {{{hex_to_c_array(v["encoded"])}}}}},')
    out.append("    };")
    out.append("")

    # from_uniform_bytes
    fu_vecs = rist.get("from_uniform_bytes", [])
    out.append(f"    static constexpr int RISTRETTO_FU_COUNT = {len(fu_vecs)};")
    out.append("    struct RistrettoFuVec { const char *label; unsigned char input[64]; unsigned char result[32]; };")
    out.append("    static constexpr RistrettoFuVec ristretto_fu_vectors[] = {")
    for v in fu_vecs:
        out.append(f'        {{"{v["label"]}",')
        out.append(f'         {{{hex_to_c_array(v["input"])}}},')
        out.append(f'         {{{hex_to_c_array(v["result"])}}}}},')
    out.append("    };")
    out.append("")


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.json> <output.h>", file=sys.stderr)
        sys.exit(1)

    with open(sys.argv[1], 'r') as f:
        data = json.load(f)

    out = []
    out.append("/**")
    out.append("This is free and unencumbered software released into the public domain.")
    out.append("")
    out.append("AUTO-GENERATED by tools/json_to_header.py — DO NOT EDIT MANUALLY.")
    out.append(f"Source: {sys.argv[1]}")
    out.append(f"Generator: {data.get('generator', 'unknown')}")
    out.append(f"Version: {data.get('version', '?')}")
    out.append("*/")
    out.append("")
    out.append("#ifndef ED25519_TEST_VECTORS_H")
    out.append("#define ED25519_TEST_VECTORS_H")
    out.append("")
    out.append("#include <cstdint>")
    out.append("")
    out.append("namespace generated_vectors")
    out.append("{")

    emit_scalar_section(out, data.get("scalars", {}))
    emit_fe_section(out, data.get("field_elements", {}))
    emit_ge_section(out, data.get("group_elements", {}))
    emit_ristretto_section(out, data.get("ristretto255", {}))

    out.append("} // namespace generated_vectors")
    out.append("")
    out.append("#endif // ED25519_TEST_VECTORS_H")
    out.append("")

    with open(sys.argv[2], 'w', newline='\n') as f:
        f.write('\n'.join(out))

    print(f"Generated {sys.argv[2]} ({len(out)} lines)")


if __name__ == "__main__":
    main()
