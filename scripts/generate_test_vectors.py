#!/usr/bin/env python3
"""
Generate Ed25519 test vectors for validating the C++ library.

Uses the DJB reference Ed25519 implementation (pure Python) and
cross-validates against PyNaCl (libsodium) when available.

Output: prints C++ constant arrays with hardcoded test vectors.
"""

import hashlib
import sys

sys.setrecursionlimit(10000)

# ============================================================
# Ed25519 Constants and Core Implementation
# (based on DJB reference: https://ed25519.cr.yp.to/python/ed25519.py)
# ============================================================

b = 256
q = 2**255 - 19  # field prime
l = 2**252 + 27742317777372353535851937790883648493  # group order

def inv(x):
    return pow(x, q - 2, q)

d = -121665 * inv(121666) % q
I_val = pow(2, (q - 1) // 4, q)

def xrecover(y):
    xx = (y * y - 1) * inv(d * y * y + 1)
    x = pow(xx, (q + 3) // 8, q)
    if (x * x - xx) % q != 0:
        x = (x * I_val) % q
    if x % 2 != 0:
        x = q - x
    return x

By = 4 * inv(5) % q
Bx = xrecover(By)
B = (Bx % q, By % q)

def edwards(P, Q):
    x1, y1 = P
    x2, y2 = Q
    x3 = ((x1 * y2 + x2 * y1) * pow(1 + d * x1 * x2 * y1 * y2, -1, q)) % q
    y3 = ((y1 * y2 + x1 * x2) * pow(1 - d * x1 * x2 * y1 * y2, -1, q)) % q
    return (x3, y3)

def scalarmult(P, e):
    R = (0, 1)
    Q = P
    while e > 0:
        if e & 1:
            R = edwards(R, Q)
        Q = edwards(Q, Q)
        e >>= 1
    return R

def encodepoint(P):
    x, y = P
    bits = [(y >> i) & 1 for i in range(b - 1)] + [x & 1]
    return bytes([sum([bits[i * 8 + j] << j for j in range(8)]) for i in range(b // 8)])

def bit(h, i):
    return (h[i // 8] >> (i % 8)) & 1

def isoncurve(P):
    x, y = P
    return (-x * x + y * y - 1 - d * x * x * y * y) % q == 0

def publickey_from_seed(sk):
    """Derive (clamped_scalar_int, pubkey_bytes) from 32-byte seed."""
    h = hashlib.sha512(sk).digest()
    a = 2**(b - 2) + sum(2**i * bit(h, i) for i in range(3, b - 2))
    A = scalarmult(B, a)
    return a, encodepoint(A)


# ============================================================
# Self-checks
# ============================================================

assert isoncurve(B), "Base point not on curve"

G_encoded = encodepoint(B)
assert G_encoded == bytes.fromhex(
    "5866666666666666666666666666666666666666666666666666666666666666"
), f"Base point encoding mismatch: {G_encoded.hex()}"

# Cross-validate against PyNaCl if available
try:
    import nacl.signing
    for seed_hex in [
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
        "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
        "c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7",
    ]:
        seed = bytes.fromhex(seed_hex)
        nacl_pubkey = bytes(nacl.signing.SigningKey(seed).verify_key)
        _, my_pubkey = publickey_from_seed(seed)
        assert nacl_pubkey == my_pubkey, \
            f"Cross-validation failed for seed {seed_hex}: nacl={nacl_pubkey.hex()} mine={my_pubkey.hex()}"
    print("Cross-validation with PyNaCl: PASSED", file=sys.stderr)
except ImportError:
    print("PyNaCl not available, skipping cross-validation", file=sys.stderr)


# ============================================================
# Helpers
# ============================================================

def int_to_bytes32(v):
    return (v % (1 << 256)).to_bytes(32, 'little')

def bytes32_to_int(bs):
    return int.from_bytes(bs, 'little')

def int_to_bytes64(v):
    return (v % (1 << 512)).to_bytes(64, 'little')

def print_bytes(name, bs, indent="    "):
    hex_str = ", ".join(f"0x{byte:02x}" for byte in bs)
    print(f"{indent}const unsigned char {name}[] = {{{hex_str}}};")


# ============================================================
# Generate Test Vectors
# ============================================================

# --- Scalar Arithmetic ---

scalar_a_int = 0x04985da8a8560911369636dff2d227f5f62439f44c8f4fb902a02b28843f3b31
scalar_b_int = 0x02e3a9d08d47c83fa42b1a6e0e2cb0c3e84f16b1d9a7f4031bbcd2045a913c18
scalar_c_int = 0x0699f55c121a0dc9ed2e47de80fe94aadb3ecc74bf5e1d0b43c68a9e51ed7f04

assert scalar_a_int < l
assert scalar_b_int < l
assert scalar_c_int < l

scalar_a = int_to_bytes32(scalar_a_int)
scalar_b = int_to_bytes32(scalar_b_int)
scalar_c = int_to_bytes32(scalar_c_int)

sc_add_result = int_to_bytes32((scalar_a_int + scalar_b_int) % l)
sc_sub_result = int_to_bytes32((scalar_a_int - scalar_b_int) % l)
sc_mul_result = int_to_bytes32((scalar_a_int * scalar_b_int) % l)
sc_muladd_result = int_to_bytes32((scalar_c_int + scalar_a_int * scalar_b_int) % l)
sc_mulsub_result = int_to_bytes32((scalar_c_int - scalar_a_int * scalar_b_int) % l)

# sc_reduce32
scalar_over_l = l + 12345
sc_reduce32_input = int_to_bytes32(scalar_over_l)
sc_reduce32_result = int_to_bytes32(scalar_over_l % l)

scalar_over_l2 = (1 << 255) - 1
sc_reduce32_input2 = int_to_bytes32(scalar_over_l2)
sc_reduce32_result2 = int_to_bytes32(scalar_over_l2 % l)

# sc_reduce (64-byte input)
scalar_64_int = scalar_a_int * scalar_b_int
sc_reduce_input = int_to_bytes64(scalar_64_int)
sc_reduce_result = int_to_bytes32(scalar_64_int % l)

scalar_64_int2 = (1 << 511) - 1
sc_reduce_input2 = int_to_bytes64(scalar_64_int2)
sc_reduce_result2 = int_to_bytes32(scalar_64_int2 % l)

# sc_check
sc_check_valid_1 = scalar_a
sc_check_valid_2 = int_to_bytes32(0)
sc_check_valid_3 = int_to_bytes32(1)
sc_check_valid_4 = int_to_bytes32(l - 1)
sc_check_invalid_1 = int_to_bytes32(l)
sc_check_invalid_2 = int_to_bytes32(l + 1)
sc_check_invalid_3 = int_to_bytes32((1 << 256) - 1)

# sc_isnonzero
sc_isnonzero_zero = int_to_bytes32(0)
sc_isnonzero_one = int_to_bytes32(1)
sc_isnonzero_a = scalar_a

# --- Field Element Arithmetic ---

p = q

fe_rt_input_1 = int_to_bytes32(42)
fe_rt_expected_1 = int_to_bytes32(42)

fe_rt_input_2 = int_to_bytes32(p - 1)
fe_rt_expected_2 = int_to_bytes32(p - 1)

fe_rt_input_3 = int_to_bytes32(p)
fe_rt_expected_3 = int_to_bytes32(0)

fe_rt_input_4 = int_to_bytes32(p + 1)
fe_rt_expected_4 = int_to_bytes32(1)

fe_rt_input_5_int = (1 << 255) + 100
fe_rt_input_5 = int_to_bytes32(fe_rt_input_5_int)
fe_rt_expected_5 = int_to_bytes32(100)

fe_a_int = 0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef & ((1 << 255) - 1)
fe_b_int = 0x7edcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321 & ((1 << 255) - 1)
fe_a_int = fe_a_int % p
fe_b_int = fe_b_int % p

fe_a = int_to_bytes32(fe_a_int)
fe_b = int_to_bytes32(fe_b_int)
fe_add_expected = int_to_bytes32((fe_a_int + fe_b_int) % p)
fe_sub_expected = int_to_bytes32((fe_a_int - fe_b_int) % p)
fe_mul_expected = int_to_bytes32((fe_a_int * fe_b_int) % p)
fe_sq_a_expected = int_to_bytes32((fe_a_int * fe_a_int) % p)
fe_sq2_a_expected = int_to_bytes32((2 * fe_a_int * fe_a_int) % p)
fe_neg_a_expected = int_to_bytes32((-fe_a_int) % p)
fe_invert_a_expected = int_to_bytes32(pow(fe_a_int, -1, p))

assert (fe_a_int * pow(fe_a_int, -1, p)) % p == 1

# --- Group Element Operations ---

# Derive (scalar, pubkey) from RFC 8032 seeds
seed1 = bytes.fromhex("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60")
scalar1_int, pubkey1 = publickey_from_seed(seed1)
scalar1 = int_to_bytes32(scalar1_int)

seed2 = bytes.fromhex("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb")
scalar2_int, pubkey2 = publickey_from_seed(seed2)
scalar2 = int_to_bytes32(scalar2_int)

seed3 = bytes.fromhex("c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7")
scalar3_int, pubkey3 = publickey_from_seed(seed3)
scalar3 = int_to_bytes32(scalar3_int)

# scalar = 1 * G = base point
scalar_one = int_to_bytes32(1)
pubkey_one = encodepoint(scalarmult(B, 1))
assert pubkey_one == G_encoded

# ge_frombytes_negate_vartime: decode negates, re-encoding gives flipped sign bit
negate1 = bytearray(pubkey1); negate1[31] ^= 0x80; negate1 = bytes(negate1)
negate2 = bytearray(pubkey2); negate2[31] ^= 0x80; negate2 = bytes(negate2)
negateG = bytearray(G_encoded); negateG[31] ^= 0x80; negateG = bytes(negateG)

# --- ge_frombytes_vartime: decode (no negate) roundtrip ---
frombytes_point1 = pubkey1
frombytes_point2 = pubkey2
frombytes_point3 = G_encoded

# --- ge_fromfe_frombytes_vartime: Elligator-like mapping ---

A_mont = 486662

def compute_even_sqrt(a):
    """Compute the even square root of a mod q. q = 5 (mod 8)."""
    a = a % q
    if a == 0:
        return 0
    x = pow(a, (q + 3) // 8, q)
    if (x * x) % q != a:
        x = (x * I_val) % q
    assert (x * x) % q == a, f"No sqrt for {a}"
    if x % 2 != 0:
        x = q - x
    return x

fffb1_val = compute_even_sqrt((-2 * A_mont * (A_mont + 2)) % q)
fffb2_val = compute_even_sqrt((2 * A_mont * (A_mont + 2)) % q)
fffb3_val = compute_even_sqrt((-I_val * A_mont * (A_mont + 2)) % q)
fffb4_val = compute_even_sqrt((I_val * A_mont * (A_mont + 2)) % q)

def divpowm1_py(u, v):
    """Compute (u/v)^((q+3)/8) = u * v^3 * (u * v^7)^((q-5)/8) mod q"""
    v3 = pow(v, 3, q)
    v7 = pow(v, 7, q)
    uv7 = (u * v7) % q
    return (u * v3 % q * pow(uv7, (q - 5) // 8, q)) % q

def ge_fromfe_mapping(s_bytes, negate):
    """
    Python implementation of ge_fromfe_frombytes_[negate_]vartime.
    Returns encoded 32-byte point.
    """
    u = int.from_bytes(s_bytes, 'little') & ((1 << 255) - 1)
    u = u % q

    v = (2 * u * u) % q
    w = (v + 1) % q
    x_orig = (w * w + (-A_mont * A_mont % q) * v) % q

    rX = divpowm1_py(w, x_orig)
    x_check = (rX * rX % q * x_orig) % q
    z = (-A_mont) % q

    took_neg = False
    if (w - x_check) % q != 0:
        if (w + x_check) % q != 0:
            took_neg = True
        else:
            rX = (rX * fffb1_val) % q
    else:
        rX = (rX * fffb2_val) % q

    if not took_neg:
        rX = (rX * u) % q
        z = (z * v) % q
        sign = 0 if negate else 1
    else:
        x_neg = (x_check * I_val) % q
        if (w - x_neg) % q != 0:
            rX = (rX * fffb3_val) % q
        else:
            rX = (rX * fffb4_val) % q
        sign = 1 if negate else 0

    if (rX % 2) != sign:
        rX = (q - rX) % q

    Z = (z + w) % q
    Y = (z - w) % q
    X = (rX * Z) % q

    Zinv = pow(Z, q - 2, q)
    xa = (X * Zinv) % q
    ya = (Y * Zinv) % q
    assert (-xa * xa + ya * ya - 1 - d * xa * xa * ya * ya) % q == 0, "Point not on curve!"

    return encodepoint((xa, ya))

fromfe_input1 = G_encoded
fromfe_input2 = pubkey1
fromfe_input3 = pubkey2

fromfe_out1 = ge_fromfe_mapping(fromfe_input1, negate=False)
fromfe_out2 = ge_fromfe_mapping(fromfe_input2, negate=False)
fromfe_out3 = ge_fromfe_mapping(fromfe_input3, negate=False)

fromfe_negate_out1 = ge_fromfe_mapping(fromfe_input1, negate=True)
fromfe_negate_out2 = ge_fromfe_mapping(fromfe_input2, negate=True)
fromfe_negate_out3 = ge_fromfe_mapping(fromfe_input3, negate=True)

# Cross-validate: negate and non-negate outputs differ only in sign bit
for i, (neg, pos) in enumerate([
    (fromfe_negate_out1, fromfe_out1),
    (fromfe_negate_out2, fromfe_out2),
    (fromfe_negate_out3, fromfe_out3),
]):
    flipped = bytearray(neg); flipped[31] ^= 0x80
    assert bytes(flipped) == pos, \
        f"Cross-check failed for fromfe input {i+1}: neg={neg.hex()}, pos={pos.hex()}"

print("Elligator mapping cross-validation: PASSED", file=sys.stderr)


# ============================================================
# Output C++ test vectors
# ============================================================

print("// ==============================================")
print("// AUTO-GENERATED TEST VECTORS - DO NOT MODIFY")
print("// Generated by scripts/generate_test_vectors.py")
print("// These values are LOCKED and must not be changed")
print("// without explicit authorization.")
print("// ==============================================")
print()
print("namespace test_vectors")
print("{")

print("    // --- Scalar Test Inputs ---")
print_bytes("scalar_a", scalar_a)
print_bytes("scalar_b", scalar_b)
print_bytes("scalar_c", scalar_c)
print()

print("    // --- sc_add: (a + b) mod l ---")
print_bytes("sc_add_ab", sc_add_result)
print()

print("    // --- sc_sub: (a - b) mod l ---")
print_bytes("sc_sub_ab", sc_sub_result)
print()

print("    // --- sc_mul: (a * b) mod l ---")
print_bytes("sc_mul_ab", sc_mul_result)
print()

print("    // --- sc_muladd: (c + a*b) mod l ---")
print_bytes("sc_muladd_abc", sc_muladd_result)
print()

print("    // --- sc_mulsub: (c - a*b) mod l ---")
print_bytes("sc_mulsub_abc", sc_mulsub_result)
print()

print("    // --- sc_reduce32: reduce 32-byte value mod l ---")
print_bytes("sc_reduce32_in1", sc_reduce32_input)
print_bytes("sc_reduce32_out1", sc_reduce32_result)
print_bytes("sc_reduce32_in2", sc_reduce32_input2)
print_bytes("sc_reduce32_out2", sc_reduce32_result2)
print()

print("    // --- sc_reduce: reduce 64-byte value mod l ---")
print_bytes("sc_reduce_in1", sc_reduce_input)
print_bytes("sc_reduce_out1", sc_reduce_result)
print_bytes("sc_reduce_in2", sc_reduce_input2)
print_bytes("sc_reduce_out2", sc_reduce_result2)
print()

print("    // --- sc_check: 0 = valid (< l), non-zero = invalid ---")
print_bytes("sc_check_valid_1", sc_check_valid_1)
print_bytes("sc_check_valid_2", sc_check_valid_2)
print_bytes("sc_check_valid_3", sc_check_valid_3)
print_bytes("sc_check_valid_4", sc_check_valid_4)
print_bytes("sc_check_invalid_1", sc_check_invalid_1)
print_bytes("sc_check_invalid_2", sc_check_invalid_2)
print_bytes("sc_check_invalid_3", sc_check_invalid_3)
print()

print("    // --- sc_isnonzero ---")
print_bytes("sc_isnonzero_zero", sc_isnonzero_zero)
print_bytes("sc_isnonzero_one", sc_isnonzero_one)
print_bytes("sc_isnonzero_a", sc_isnonzero_a)
print()

print("    // --- Field element roundtrip: tobytes(frombytes(x)) ---")
print_bytes("fe_rt_in1", fe_rt_input_1)
print_bytes("fe_rt_out1", fe_rt_expected_1)
print_bytes("fe_rt_in2", fe_rt_input_2)
print_bytes("fe_rt_out2", fe_rt_expected_2)
print_bytes("fe_rt_in3", fe_rt_input_3)
print_bytes("fe_rt_out3", fe_rt_expected_3)
print_bytes("fe_rt_in4", fe_rt_input_4)
print_bytes("fe_rt_out4", fe_rt_expected_4)
print_bytes("fe_rt_in5", fe_rt_input_5)
print_bytes("fe_rt_out5", fe_rt_expected_5)
print()

print("    // --- Field element arithmetic ---")
print_bytes("fe_a", fe_a)
print_bytes("fe_b", fe_b)
print_bytes("fe_add_ab", fe_add_expected)
print_bytes("fe_sub_ab", fe_sub_expected)
print_bytes("fe_mul_ab", fe_mul_expected)
print_bytes("fe_sq_a", fe_sq_a_expected)
print_bytes("fe_sq2_a", fe_sq2_a_expected)
print_bytes("fe_neg_a", fe_neg_a_expected)
print_bytes("fe_invert_a", fe_invert_a_expected)
print()

print("    // --- ge_scalarmult_base: scalar * G ---")
print("    // Derived from RFC 8032 seed 1")
print_bytes("scalarmult_scalar1", scalar1)
print_bytes("scalarmult_expected1", pubkey1)
print("    // Derived from RFC 8032 seed 2")
print_bytes("scalarmult_scalar2", scalar2)
print_bytes("scalarmult_expected2", pubkey2)
print("    // Derived from RFC 8032 seed 3")
print_bytes("scalarmult_scalar3", scalar3)
print_bytes("scalarmult_expected3", pubkey3)
print("    // scalar = 1 (should give base point)")
print_bytes("scalarmult_scalar_one", scalar_one)
print_bytes("scalarmult_expected_one", pubkey_one)
print()

print("    // --- ge_frombytes_negate_vartime + ge_p3_tobytes ---")
print("    // Input point -> decode+negate -> encode = flipped sign bit")
print_bytes("negate_point_in1", pubkey1)
print_bytes("negate_point_out1", negate1)
print_bytes("negate_point_in2", pubkey2)
print_bytes("negate_point_out2", negate2)
print_bytes("negate_point_in3", G_encoded)
print_bytes("negate_point_out3", negateG)
print()

print("    // --- ge_frombytes_vartime + ge_p3_tobytes ---")
print("    // Input point -> decode (no negate) -> encode = same bytes (roundtrip)")
print_bytes("frombytes_point1", frombytes_point1)
print_bytes("frombytes_point2", frombytes_point2)
print_bytes("frombytes_point3", frombytes_point3)
print()

print("    // --- ge_fromfe_frombytes_vartime + ge_tobytes ---")
print("    // Elligator-like mapping: field element bytes -> curve point")
print_bytes("fromfe_in1", fromfe_input1)
print_bytes("fromfe_out1", fromfe_out1)
print_bytes("fromfe_negate_out1", fromfe_negate_out1)
print_bytes("fromfe_in2", fromfe_input2)
print_bytes("fromfe_out2", fromfe_out2)
print_bytes("fromfe_negate_out2", fromfe_negate_out2)
print_bytes("fromfe_in3", fromfe_input3)
print_bytes("fromfe_out3", fromfe_out3)
print_bytes("fromfe_negate_out3", fromfe_negate_out3)
print()

print("} // namespace test_vectors")
