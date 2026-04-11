cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug, Release, RelWithDebInfo" FORCE)
endif()
message(STATUS "Mode: ${CMAKE_BUILD_TYPE}")

set(LIB_MAJOR_VERSION "2")
set(LIB_MINOR_VERSION "0")
set(LIB_PATCH_VERSION "0")
set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

project(ed25519 VERSION "${LIB_VERSION_STRING}" LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

# --- IDE and build ergonomics (only when this is the top-level project) ---
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(NOT MSVC)
        find_program(CCACHE_PROGRAM ccache)
        if(CCACHE_PROGRAM)
            message(STATUS "Found ccache... Activating...")
            set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        endif()
    endif()

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_SUPPRESS_REGENERATION ON)
endif()

# --- Compiler feature/flag detection (sets cache variables only) ---
option(ED25519_ENABLE_SPECTRE_MITIGATIONS "Enable MSVC Spectre mitigations (/Qspectre)" OFF)
if(MSVC)
    if(ED25519_ENABLE_SPECTRE_MITIGATIONS)
        check_cxx_compiler_flag("/Qspectre" HAS_QSPECTRE)
    endif()
    # CET shadow stack compatibility
    check_cxx_compiler_flag("/guard:ehcont" HAS_GUARD_EHCONT)
else()
    set(CMAKE_REQUIRED_FLAGS "-fstack-protector-strong")
    check_cxx_source_compiles("int main() { char buf[64]; buf[0] = 0; return buf[0]; }" HAS_STACK_PROTECTOR_STRONG)
    unset(CMAKE_REQUIRED_FLAGS)

    # Intel CET control flow integrity (x86_64 only)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64)$")
        check_cxx_compiler_flag("-fcf-protection=full" HAS_CF_PROTECTION)
    endif()
endif()

set(ED25519_SRC
    src/ed25519_secure_erase.cpp
    src/ge.cpp
    src/ristretto255.cpp
    src/x25519.cpp
    src/slide.cpp
    src/portable/sc_mul.cpp
    src/portable/sc_muladd.cpp
    src/portable/sc_mulsub.cpp
    src/portable/sc_reduce.cpp
)

option(ED25519_FORCE_PORTABLE "Force portable (32-bit) implementation even on 64-bit platforms" OFF)

# Detect x86_64 specifically (SIMD backends are x86_64-only: AVX2, AVX-512 IFMA)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64)$")
    set(ED25519_IS_X86_64 TRUE)
else()
    set(ED25519_IS_X86_64 FALSE)
endif()

# SIMD backend options (x86_64 only, default ON when applicable)
# Set either to OFF to benchmark individual code paths in isolation.
# When both are OFF, produces the exact same binary as pre-SIMD builds.
if(ED25519_IS_X86_64 AND NOT ED25519_FORCE_PORTABLE)
    option(ED25519_ENABLE_AVX512 "Enable AVX-512 IFMA backend (set OFF to test AVX2 on AVX-512 hardware)" ON)
    option(ED25519_ENABLE_AVX2 "Enable AVX2 backend (set OFF to test x64 baseline on AVX2+ hardware)" ON)
else()
    option(ED25519_ENABLE_AVX512 "Enable AVX-512 IFMA backend" OFF)
    option(ED25519_ENABLE_AVX2 "Enable AVX2 backend" OFF)
endif()

# Force SIMD off when portable is active — option() only sets defaults and won't
# override explicit -DED25519_ENABLE_AVX2=ON or cached values from a parent project.
if(ED25519_FORCE_PORTABLE)
    set(ED25519_ENABLE_AVX512 OFF CACHE BOOL "" FORCE)
    set(ED25519_ENABLE_AVX2 OFF CACHE BOOL "" FORCE)
endif()

if(NOT ED25519_FORCE_PORTABLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[aA][rR][mM]64|[aA][aA][rR][cC][hH]64)$")
    set(ED25519_X64_SRC
        src/x64/fe_divpowm1.cpp
        src/x64/fe_frombytes.cpp
        src/x64/fe_invert.cpp
        src/x64/fe_isnegative.cpp
        src/x64/fe_isnonzero.cpp
        src/x64/fe_mul.cpp
        src/x64/fe_mul121666.cpp
        src/x64/fe_pow22523.cpp
        src/x64/fe_sq.cpp
        src/x64/fe_sq2.cpp
        src/x64/fe_sqn.cpp
        src/x64/fe_tobytes.cpp
        src/x64/ge_add.cpp
        src/x64/ge_check_subgroup_precomp_negate_vartime.cpp
        src/x64/ge_check_subgroup_precomp_vartime.cpp
        src/x64/ge_double_scalarmult_base_negate_vartime.cpp
        src/x64/ge_double_scalarmult_negate_vartime.cpp
        src/x64/ge_dsm_precomp.cpp
        src/x64/ge_frombytes_vartime.cpp
        src/x64/ge_fromfe_frombytes_vartime.cpp
        src/x64/ge_madd.cpp
        src/x64/ge_msub.cpp
        src/x64/ge_p2_dbl.cpp
        src/x64/ge_p2_to_p3.cpp
        src/x64/ge_p3_tobytes.cpp
        src/x64/ge_p3_to_wei25519.cpp
        src/x64/ge_scalarmult_base_ct.cpp
        src/x64/ge_scalarmult_ct.cpp
        src/x64/ge_scalarmult_ct_batch.cpp
        src/x64/ge_dsm_base_negate_vt_batch.cpp
        src/x64/ge_dsm_negate_vt_batch_ss.cpp
        src/x64/ge_dsm_negate_vt_batch_ss_p3.cpp
        src/x64/ge_multiscalar_mul_vartime.cpp
        src/x64/ge_sub.cpp
        src/x64/ge_tobytes.cpp
        src/x64/sc_add.cpp
        src/x64/sc_check_reduced.cpp
        src/x64/sc_mul.cpp
        src/x64/sc_muladd.cpp
        src/x64/sc_mulsub.cpp
        src/x64/sc_reduce.cpp
        src/x64/sc_sub.cpp
    )
    list(APPEND ED25519_SRC ${ED25519_X64_SRC})
    message(STATUS "ED25519: 64-bit optimized field arithmetic enabled (radix-2^51)")
else()
    set(ED25519_PORTABLE_SRC
        src/portable/fe_divpowm1.cpp
        src/portable/fe_frombytes.cpp
        src/portable/fe_invert.cpp
        src/portable/fe_isnegative.cpp
        src/portable/fe_isnonzero.cpp
        src/portable/fe_mul.cpp
        src/portable/fe_mul121666.cpp
        src/portable/fe_pow22523.cpp
        src/portable/fe_sq.cpp
        src/portable/fe_sq2.cpp
        src/portable/fe_sqn.cpp
        src/portable/fe_tobytes.cpp
        src/portable/ge_check_subgroup_precomp_negate_vartime.cpp
        src/portable/ge_check_subgroup_precomp_vartime.cpp
        src/portable/ge_double_scalarmult_base_negate_vartime.cpp
        src/portable/ge_double_scalarmult_negate_vartime.cpp
        src/portable/ge_dsm_precomp.cpp
        src/portable/ge_frombytes_vartime.cpp
        src/portable/ge_fromfe_frombytes_vartime.cpp
        src/portable/ge_p2_to_p3.cpp
        src/portable/ge_p3_tobytes.cpp
        src/portable/ge_p3_to_wei25519.cpp
        src/portable/ge_scalarmult_base_ct.cpp
        src/portable/ge_scalarmult_ct.cpp
        src/portable/ge_scalarmult_ct_batch.cpp
        src/portable/ge_dsm_base_negate_vt_batch.cpp
        src/portable/ge_dsm_negate_vt_batch_ss.cpp
        src/portable/ge_dsm_negate_vt_batch_ss_p3.cpp
        src/portable/ge_multiscalar_mul_vartime.cpp
        src/portable/ge_tobytes.cpp
        src/portable/sc_add.cpp
        src/portable/sc_check_reduced.cpp
        src/portable/sc_sub.cpp
    )
    list(APPEND ED25519_SRC ${ED25519_PORTABLE_SRC})
    message(STATUS "ED25519: 32-bit portable implementation")
endif()

if(ED25519_FORCE_PORTABLE)
    message(STATUS "ED25519: Forced portable (32-bit) implementation via ED25519_FORCE_PORTABLE")
endif()

# SIMD support: derived from ED25519_ENABLE_AVX2 / ED25519_ENABLE_AVX512
# When either backend is enabled, compile CPUID module and define ED25519_SIMD=1
if(ED25519_ENABLE_AVX2 OR ED25519_ENABLE_AVX512)
    # CPUID module and dispatch table: plain integer code, no special flags needed
    list(APPEND ED25519_SRC src/cpuid.cpp src/dispatch.cpp)

    # AVX-512 IFMA backend translation units (Phase 2)
    if(ED25519_ENABLE_AVX512)
        set(ED25519_IFMA_SRC
            src/x64/ifma/ge_scalarmult_ct.cpp
            src/x64/ifma/ge_scalarmult_base_ct.cpp
            src/x64/ifma/ge_double_scalarmult_base_negate_vartime.cpp
            src/x64/ifma/ge_double_scalarmult_negate_vartime.cpp
            src/x64/ifma/ge_scalarmult_ct_batch.cpp
            src/x64/ifma/ge_dsm_base_negate_vt_batch.cpp
            src/x64/ifma/ge_dsm_negate_vt_batch_ss.cpp
            src/x64/ifma/ge_dsm_negate_vt_batch_ss_p3.cpp
            src/x64/ifma/ge_multiscalar_mul_vartime.cpp
        )
        list(APPEND ED25519_SRC ${ED25519_IFMA_SRC})
    endif()

    # AVX2 backend translation units (Phase 3)
    if(ED25519_ENABLE_AVX2)
        set(ED25519_AVX2_SRC
            src/x64/avx2/ge_scalarmult_ct.cpp
            src/x64/avx2/ge_scalarmult_base_ct.cpp
            src/x64/avx2/ge_scalarmult_ct_batch.cpp
            src/x64/avx2/ge_dsm_base_negate_vt_batch.cpp
            src/x64/avx2/ge_dsm_negate_vt_batch_ss.cpp
            src/x64/avx2/ge_dsm_negate_vt_batch_ss_p3.cpp
            src/x64/avx2/ge_multiscalar_mul_vartime.cpp
        )
        list(APPEND ED25519_SRC ${ED25519_AVX2_SRC})
    endif()

    # Status messages
    set(_simd_backends "")
    if(ED25519_ENABLE_AVX2)
        string(APPEND _simd_backends "AVX2")
    endif()
    if(ED25519_ENABLE_AVX512)
        if(_simd_backends)
            string(APPEND _simd_backends ", ")
        endif()
        string(APPEND _simd_backends "AVX-512 IFMA")
    endif()
    message(STATUS "ED25519: SIMD runtime dispatch enabled (${_simd_backends})")
else()
    message(STATUS "ED25519: SIMD runtime dispatch disabled (x64 baseline only)")
endif()

# --- Library target ---
add_library(ed25519 STATIC ${ED25519_SRC})
target_include_directories(ed25519 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# Consumers treat our headers as system headers (suppresses their warnings)
target_include_directories(ed25519 SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
set_target_properties(ed25519 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(ED25519_FORCE_PORTABLE)
    target_compile_definitions(ed25519 PUBLIC ED25519_FORCE_PORTABLE=1)
endif()
if(ED25519_ENABLE_AVX2 OR ED25519_ENABLE_AVX512)
    target_compile_definitions(ed25519 PUBLIC ED25519_SIMD=1)

    # MinGW GCC + Debug: Windows x64 ABI only guarantees 16-byte stack alignment,
    # but YMM/ZMM registers need 32/64-byte alignment. At -O0, GCC spills AVX
    # registers to the stack with aligned moves (vmovdqa), which SIGSEGV on the
    # misaligned stack. Force -O1 for SIMD backends in Debug mode so GCC keeps
    # values in registers.
    set(_MINGW_SIMD_DEBUG_FIX "")
    if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(_MINGW_SIMD_DEBUG_FIX "$<$<CONFIG:Debug>:-O1>")
    endif()

    # AVX-512 IFMA compile flags: IFMA TUs need special instruction set flags
    if(ED25519_ENABLE_AVX512)
        if(MSVC)
            set(ED25519_IFMA_FLAGS "/arch:AVX512")
        else()
            set(ED25519_IFMA_FLAGS "-mavx512f;-mavx512ifma")
            if(_MINGW_SIMD_DEBUG_FIX)
                list(APPEND ED25519_IFMA_FLAGS "${_MINGW_SIMD_DEBUG_FIX}")
            endif()
        endif()
        foreach(IFMA_SRC ${ED25519_IFMA_SRC})
            set_source_files_properties(${IFMA_SRC} PROPERTIES COMPILE_OPTIONS "${ED25519_IFMA_FLAGS}")
        endforeach()
        message(STATUS "ED25519: IFMA TUs compiled with: ${ED25519_IFMA_FLAGS}")
    else()
        target_compile_definitions(ed25519 PUBLIC ED25519_NO_AVX512=1)
        message(STATUS "ED25519: AVX-512 IFMA backend disabled (ED25519_ENABLE_AVX512=OFF)")
    endif()

    # AVX2 compile flags: AVX2 TUs need AVX2 instruction set flags
    if(ED25519_ENABLE_AVX2)
        if(MSVC)
            set(ED25519_AVX2_FLAGS "/arch:AVX2")
        else()
            set(ED25519_AVX2_FLAGS "-mavx2")
            if(_MINGW_SIMD_DEBUG_FIX)
                list(APPEND ED25519_AVX2_FLAGS "${_MINGW_SIMD_DEBUG_FIX}")
            endif()
        endif()
        foreach(AVX2_SRC ${ED25519_AVX2_SRC})
            set_source_files_properties(${AVX2_SRC} PROPERTIES COMPILE_OPTIONS "${ED25519_AVX2_FLAGS}")
        endforeach()
        message(STATUS "ED25519: AVX2 TUs compiled with: ${ED25519_AVX2_FLAGS}")
    else()
        target_compile_definitions(ed25519 PUBLIC ED25519_NO_AVX2=1)
        message(STATUS "ED25519: AVX2 backend disabled (ED25519_ENABLE_AVX2=OFF)")
    endif()
endif()

# --- Library compile options (PRIVATE — do not leak to downstream consumers) ---
if(MSVC)
    # CMake injects /W3 by default; strip it to avoid D9025 "overriding /W3 with /W4"
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    target_compile_options(ed25519 PRIVATE
        /W4 /GS /guard:cf /sdl /wd4267 /wd4804
        $<$<CONFIG:Debug>:/Ob2 /Oi>
    )
    # /wd4267: size_t->int truncation in fe arithmetic (bounded by algorithm, always < limb count)
    # /wd4804: bool in constant-time bit operations (intentional branchless idiom)
    if(HAS_QSPECTRE)
        target_compile_options(ed25519 PRIVATE /Qspectre)
    endif()
    if(HAS_GUARD_EHCONT)
        target_compile_options(ed25519 PRIVATE /guard:ehcont)
    endif()
else()
    target_compile_options(ed25519 PRIVATE
        -Wall -Wextra -Wuninitialized
        $<$<CONFIG:Debug>:-g3 -Og>
    )
    if(HAS_STACK_PROTECTOR_STRONG)
        target_compile_options(ed25519 PRIVATE -fstack-protector-strong)
    endif()

    # Disable auto-vectorization to prevent the compiler from introducing
    # SIMD instructions that could create data-dependent timing behavior,
    # which would undermine the constant-time guarantees of the crypto code.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(ed25519 PRIVATE -fno-tree-vectorize)
        # libstdc++ container assertions in Debug (bounds checks on operator[], etc.)
        target_compile_options(ed25519 PRIVATE
            $<$<CONFIG:Debug>:-D_GLIBCXX_ASSERTIONS>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(ed25519 PRIVATE -fno-vectorize -fno-slp-vectorize)
    endif()

    if(HAS_CF_PROTECTION)
        target_compile_options(ed25519 PRIVATE -fcf-protection=full)
    endif()

    # Stack clash protection (Linux only — not supported on macOS/Apple Clang, not needed on Windows)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_options(ed25519 PRIVATE -fstack-clash-protection)
    endif()

    # Runtime buffer overflow detection via checked libc wrappers (non-Debug only)
    # Skip on Windows — MinGW doesn't have fortified libc wrappers
    if(NOT WIN32)
        target_compile_options(ed25519 PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=2>
        )
    endif()
endif()

# --- LTO (target-scoped) ---
option(ED25519_ENABLE_LTO "Enable link-time optimization" OFF)
if(ED25519_ENABLE_LTO)
    if(MSVC)
        target_compile_options(ed25519 PRIVATE /GL)
        set_target_properties(ed25519 PROPERTIES STATIC_LIBRARY_FLAGS "/LTCG")
    else()
        target_compile_options(ed25519 PRIVATE -flto)
    endif()
    message(STATUS "ED25519: Link-time optimization enabled")
endif()

# --- Sanitizer configuration (test/fuzzer-only, never touches `ed25519` target) ---
set(ED25519_TEST_SANITIZERS "" CACHE STRING
    "Comma-separated sanitizers to apply to test binaries only (e.g. address,undefined; thread). \
Not applied to the `ed25519` library target.")

# Rebuild a set of library sources into a private OBJECT library mirroring the
# `ed25519` target's include dirs / defines / compile options, plus any
# extra compile options. Used by sanitized test builds and libFuzzer harnesses
# so instrumentation never touches the installed `ed25519` library target.
function(ed25519_make_object_library name sources)
    add_library(${name} OBJECT ${sources})
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )
    target_include_directories(${name} PRIVATE
        $<TARGET_PROPERTY:ed25519,INCLUDE_DIRECTORIES>)
    target_compile_definitions(${name} PRIVATE
        $<TARGET_PROPERTY:ed25519,COMPILE_DEFINITIONS>)
    target_compile_options(${name} PRIVATE
        $<TARGET_PROPERTY:ed25519,COMPILE_OPTIONS>
        ${ARGN})
endfunction()

function(ed25519_apply_sanitizers target sanitizers_csv)
    if(NOT sanitizers_csv)
        return()
    endif()
    if(MINGW)
        message(FATAL_ERROR "ED25519_TEST_SANITIZERS is not supported on MinGW")
    endif()
    if(MSVC)
        if(NOT sanitizers_csv STREQUAL "address")
            message(FATAL_ERROR
                "MSVC only supports ED25519_TEST_SANITIZERS=address (got: ${sanitizers_csv})")
        endif()
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(FATAL_ERROR
                "MSVC ASan is incompatible with /RTC* in Debug builds — use Release or RelWithDebInfo")
        endif()
        # /guard:cf- is load-bearing: CFG's __guard_check_icall_fptr __fastfails
        # on ASan's interceptor trampoline dispatch, producing a silent
        # STATUS_STACK_BUFFER_OVERRUN at startup. /GS- /guard:ehcont- /sdl-
        # disable adjacent hardening that can surface similar failures.
        # Appended after the inherited flags so "last flag wins" disables them.
        target_compile_options(${target} PRIVATE
            /fsanitize=address /Zi
            /GS- /guard:cf- /guard:ehcont- /sdl-)
        set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS
            " /INCREMENTAL:NO /DEBUG")
    else()
        string(REPLACE "," ";" _san_list "${sanitizers_csv}")
        set(_san_cflags "")
        set(_san_ldflags "")
        foreach(_san IN LISTS _san_list)
            list(APPEND _san_cflags "-fsanitize=${_san}")
            set(_san_ldflags "${_san_ldflags} -fsanitize=${_san}")
        endforeach()
        list(APPEND _san_cflags "-fno-omit-frame-pointer" "-g")
        target_compile_options(${target} PRIVATE ${_san_cflags})
        set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS "${_san_ldflags}")
    endif()
endfunction()

# --- Helper: apply compile and linker hardening flags to executables ---
# Compile flags that affect linkage (e.g. /guard:ehcont) must be applied to
# ALL translation units in the final executable, not just the library's.
# Uses LINK_FLAGS property instead of target_link_options for CMake 3.10 compat.
function(ed25519_apply_exe_flags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /GS /guard:cf /sdl)
        set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA")
        if(HAS_GUARD_EHCONT)
            target_compile_options(${target} PRIVATE /guard:ehcont)
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " /guard:ehcont /CETCOMPAT")
        endif()
    elseif(MINGW)
        # MinGW linker hardening (DEP + ASLR) — only for GCC+MinGW (known GNU ld).
        # Clang on Windows can target either MinGW or MSVC ABI — applying the wrong
        # linker flags breaks the build. We let Clang on Windows rely on OS defaults
        # for DEP/ASLR (enabled by default on modern Windows).
        set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -static -Wl,--nxcompat -Wl,--dynamicbase -Wl,--high-entropy-va")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Linux linker hardening (for CI builds)
        set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,relro,-z,now -Wl,-z,noexecstack")
    endif()
    if(ED25519_ENABLE_LTO)
        if(MSVC)
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " /LTCG")
        else()
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -flto")
        endif()
    endif()
endfunction()

# --- Executables ---
# ED25519_BUILD_* options are forced OFF when the repo is consumed via
# add_subdirectory() by a parent project — downstream consumers never compile
# our test/benchmark/tool/fuzzer artifacts regardless of parent-cache state.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(ED25519_BUILD_TESTS "Build unit tests (ed25519-tests)" OFF)
    option(ED25519_BUILD_BENCHMARKS "Build benchmarks (ed25519-benchmark)" OFF)
    option(ED25519_BUILD_TOOLS "Build test vector generator (ed25519-gen-testvectors)" OFF)
    # dudect harnesses are local-only: upstream dudect.h is POSIX and CI
    # runners are too noisy for a reliable Welch t-test signal.
    option(ED25519_BUILD_CT_TESTS "Build empirical constant-time (dudect) harnesses" OFF)
    option(ED25519_BUILD_FUZZERS "Build libFuzzer harnesses (clang-only)" OFF)
else()
    set(ED25519_BUILD_TESTS OFF)
    set(ED25519_BUILD_BENCHMARKS OFF)
    set(ED25519_BUILD_TOOLS OFF)
    set(ED25519_BUILD_CT_TESTS OFF)
    set(ED25519_BUILD_FUZZERS OFF)
endif()

if(ED25519_BUILD_TESTS)
    message(STATUS "ED25519: Building unit tests")

    if(ED25519_TEST_SANITIZERS)
        message(STATUS "ED25519: Test binaries will be built with sanitizers: ${ED25519_TEST_SANITIZERS}")

        # Sources share `ed25519`'s set_source_files_properties() per-TU IFMA/AVX2
        # flags since they live in the same directory. The main `ed25519` target
        # is not touched — sanitizer flags stay on this target only.
        ed25519_make_object_library(ed25519_test_objs "${ED25519_SRC}")
        ed25519_apply_sanitizers(ed25519_test_objs ${ED25519_TEST_SANITIZERS})

        add_executable(ed25519-tests src/tests.cpp $<TARGET_OBJECTS:ed25519_test_objs>)
        target_include_directories(ed25519-tests PRIVATE
            $<TARGET_PROPERTY:ed25519,INTERFACE_INCLUDE_DIRECTORIES>)
        ed25519_apply_sanitizers(ed25519-tests ${ED25519_TEST_SANITIZERS})
        # ed25519_apply_exe_flags is skipped here: /GS /guard:cf /sdl /CETCOMPAT
        # conflict with ASan trampoline dispatch on MSVC (see
        # ed25519_apply_sanitizers above).
    else()
        add_executable(ed25519-tests src/tests.cpp)
        target_link_libraries(ed25519-tests PRIVATE ed25519)
        ed25519_apply_exe_flags(ed25519-tests)
    endif()
endif()

if(ED25519_BUILD_TOOLS)
    message(STATUS "ED25519: Building test vector generator")

    add_executable(ed25519-gen-testvectors tools/gen_test_vectors.cpp)
    target_link_libraries(ed25519-gen-testvectors PRIVATE ed25519)
    ed25519_apply_exe_flags(ed25519-gen-testvectors)
endif()

if(ED25519_BUILD_BENCHMARKS)
    message(STATUS "ED25519: Building benchmarks")

    add_executable(ed25519-benchmark src/benchmark.cpp)
    target_link_libraries(ed25519-benchmark PRIVATE ed25519)
    ed25519_apply_exe_flags(ed25519-benchmark)
endif()

if(ED25519_BUILD_CT_TESTS)
    if(WIN32)
        message(WARNING "ED25519_BUILD_CT_TESTS is not supported on Windows; skipping")
    else()
        message(STATUS "ED25519: Building empirical constant-time (dudect) harnesses")
        add_subdirectory(tests/ct)
    endif()
endif()

if(ED25519_BUILD_FUZZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "ED25519_BUILD_FUZZERS requires Clang (libFuzzer)")
    endif()
    message(STATUS "ED25519: Building libFuzzer harnesses")
    add_subdirectory(tests/fuzz)
endif()
