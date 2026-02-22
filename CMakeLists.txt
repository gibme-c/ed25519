cmake_minimum_required(VERSION 3.10)

if(DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: Debug, Release, RelWithDebInfo")
else()
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug, Release, RelWithDebInfo")
endif()

message(STATUS "Mode: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo Release CACHE STRING INTERNAL)
set(ARCH native CACHE STRING  "CPU to build for: -march value or native")

message(STATUS "Building for target architecture: ${ARCH}")

set(LIB_MAJOR_VERSION "2")
set(LIB_MINOR_VERSION "0")
set(LIB_PATCH_VERSION "0")
set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

cmake_policy(SET CMP0048 NEW)
project(ed25519 VERSION "${LIB_VERSION_STRING}" LANGUAGES CXX)

if(NOT MSVC)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache package... Activating...")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    endif()
endif()

## This section describes our general CMake setup options
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SKIP_INSTALL_RULES OFF FORCE)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON FORCE)
set(CMAKE_SUPPRESS_REGENERATION ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Enable c++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## We only build static binaries -- this is left here for our dependencies
set(STATIC ON CACHE BOOL FORCE "Link libraries statically? Forced to ON")
add_definitions(-DSTATICLIB)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++17 /W4 /GS /guard:cf /DWIN32_LEAN_AND_MEAN /wd4267 /wd4804 /D_DLL")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")

    string(REPLACE "/Ob0" "/Ob2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Oi")
else()
    if("${ARCH}" STREQUAL "default")
        set(ARCH_FLAG "")
    else()
        set(ARCH_FLAG "-march=${ARCH}")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAG} -std=c++17 -Wall -Wextra -Wuninitialized -fstack-protector-strong")

    # Disable auto-vectorization to prevent the compiler from introducing
    # SIMD instructions that could create data-dependent timing behavior,
    # which would undermine the constant-time guarantees of the crypto code.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-tree-vectorize")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-vectorize -fno-slp-vectorize")
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -Og")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")

    if(APPLE)
        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            # Need to build against libc++ instead of libstc++ on apple
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++ -I/usr/local/opt/llvm/include/c++/v1 -nodefaultlibs -lc++ -lc++abi -lm -lc -lgcc -lgcc_eh")

            # Need these flags so gcc OSX works on the cryptopp ASM - https://groups.google.com/forum/#!topic/cryptopp-users/po8yO-vYaac
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCRYPTOPP_CLANG_INTEGRATED_ASSEMBLER=1")

        elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
            # Need to link against the llvm libc++ library, default is too old for std::filesystem
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -I/usr/local/opt/llvm/include/c++/v1")
        endif()
    endif()

    if(MINGW OR STATIC_LIBC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()

    if(NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(ED25519_SRC
    src/ed25519_secure_erase.cpp
    src/ge.cpp
    src/ristretto255.cpp
    src/slide.cpp
    src/portable/sc_mul.cpp
    src/portable/sc_muladd.cpp
    src/portable/sc_mulsub.cpp
    src/portable/sc_reduce.cpp
)

option(FORCE_PORTABLE "Force portable (32-bit) implementation even on 64-bit platforms" OFF)

# Detect x86_64 specifically (SIMD backends are x86_64-only: AVX2, AVX-512 IFMA)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64)$")
    set(ED25519_IS_X86_64 TRUE)
else()
    set(ED25519_IS_X86_64 FALSE)
endif()

# SIMD backend options (x86_64 only, default ON when applicable)
# Set either to OFF to benchmark individual code paths in isolation.
# When both are OFF, produces the exact same binary as pre-SIMD builds.
if(ED25519_IS_X86_64 AND NOT FORCE_PORTABLE)
    option(ENABLE_AVX512 "Enable AVX-512 IFMA backend (set OFF to test AVX2 on AVX-512 hardware)" ON)
    option(ENABLE_AVX2 "Enable AVX2 backend (set OFF to test x64 baseline on AVX2+ hardware)" ON)
else()
    option(ENABLE_AVX512 "Enable AVX-512 IFMA backend" OFF)
    option(ENABLE_AVX2 "Enable AVX2 backend" OFF)
endif()

# Force SIMD off when portable is active — option() only sets defaults and won't
# override explicit -DENABLE_AVX2=ON or cached values from a parent project.
if(FORCE_PORTABLE)
    set(ENABLE_AVX512 OFF CACHE BOOL "" FORCE)
    set(ENABLE_AVX2 OFF CACHE BOOL "" FORCE)
endif()

if(NOT FORCE_PORTABLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[aA][rR][mM]64|[aA][aA][rR][cC][hH]64)$")
    set(ED25519_X64_SRC
        src/x64/fe_divpowm1.cpp
        src/x64/fe_frombytes.cpp
        src/x64/fe_invert.cpp
        src/x64/fe_isnegative.cpp
        src/x64/fe_isnonzero.cpp
        src/x64/fe_mul.cpp
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

if(FORCE_PORTABLE)
    message(STATUS "ED25519: Forced portable (32-bit) implementation via FORCE_PORTABLE")
endif()

option(ENABLE_LTO "Enable link-time optimization" OFF)
if(ENABLE_LTO)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GL")
        set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /LTCG")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
    endif()
    message(STATUS "ED25519: Link-time optimization enabled")
endif()

# SIMD support: derived from ENABLE_AVX2 / ENABLE_AVX512
# When either backend is enabled, compile CPUID module and define ED25519_SIMD=1
if(ENABLE_AVX2 OR ENABLE_AVX512)
    # CPUID module and dispatch table: plain integer code, no special flags needed
    list(APPEND ED25519_SRC src/cpuid.cpp src/dispatch.cpp)

    # AVX-512 IFMA backend translation units (Phase 2)
    if(ENABLE_AVX512)
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
    if(ENABLE_AVX2)
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
    if(ENABLE_AVX2)
        string(APPEND _simd_backends "AVX2")
    endif()
    if(ENABLE_AVX512)
        if(_simd_backends)
            string(APPEND _simd_backends ", ")
        endif()
        string(APPEND _simd_backends "AVX-512 IFMA")
    endif()
    message(STATUS "ED25519: SIMD runtime dispatch enabled (${_simd_backends})")
else()
    message(STATUS "ED25519: SIMD runtime dispatch disabled (x64 baseline only)")
endif()

add_library(ed25519 STATIC ${ED25519_SRC})
target_include_directories(ed25519 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(FORCE_PORTABLE)
    target_compile_definitions(ed25519 PUBLIC ED25519_FORCE_PORTABLE=1)
endif()
if(ENABLE_AVX2 OR ENABLE_AVX512)
    target_compile_definitions(ed25519 PUBLIC ED25519_SIMD=1)

    # AVX-512 IFMA compile flags: IFMA TUs need special instruction set flags
    if(ENABLE_AVX512)
        if(MSVC)
            set(ED25519_IFMA_FLAGS "/arch:AVX512")
        else()
            set(ED25519_IFMA_FLAGS "-mavx512f;-mavx512ifma")
        endif()
        foreach(IFMA_SRC ${ED25519_IFMA_SRC})
            set_source_files_properties(${IFMA_SRC} PROPERTIES COMPILE_OPTIONS "${ED25519_IFMA_FLAGS}")
        endforeach()
        message(STATUS "ED25519: IFMA TUs compiled with: ${ED25519_IFMA_FLAGS}")
    else()
        target_compile_definitions(ed25519 PUBLIC ED25519_NO_AVX512=1)
        message(STATUS "ED25519: AVX-512 IFMA backend disabled (ENABLE_AVX512=OFF)")
    endif()

    # AVX2 compile flags: AVX2 TUs need AVX2 instruction set flags
    if(ENABLE_AVX2)
        if(MSVC)
            set(ED25519_AVX2_FLAGS "/arch:AVX2")
        else()
            set(ED25519_AVX2_FLAGS "-mavx2")
        endif()
        foreach(AVX2_SRC ${ED25519_AVX2_SRC})
            set_source_files_properties(${AVX2_SRC} PROPERTIES COMPILE_OPTIONS "${ED25519_AVX2_FLAGS}")
        endforeach()
        message(STATUS "ED25519: AVX2 TUs compiled with: ${ED25519_AVX2_FLAGS}")
    else()
        target_compile_definitions(ed25519 PUBLIC ED25519_NO_AVX2=1)
        message(STATUS "ED25519: AVX2 backend disabled (ENABLE_AVX2=OFF)")
    endif()
endif()

option(BUILD_TESTS "Build unit tests" OFF)
if(DEFINED ENV{BUILD_TESTS})
    set(BUILD_TESTS $ENV{BUILD_TESTS})
endif()

option(BUILD_BENCHMARKS "Build benchmarks" OFF)
if(DEFINED ENV{BUILD_BENCHMARKS})
    set(BUILD_BENCHMARKS $ENV{BUILD_BENCHMARKS})
endif()

if(BUILD_TESTS)
    message(STATUS "ED25519: Building unit tests")

    add_executable(ed25519-tests src/tests.cpp)
    target_link_libraries(ed25519-tests ed25519)
endif()

option(BUILD_TOOLS "Build test vector generator" OFF)
if(DEFINED ENV{BUILD_TOOLS})
    set(BUILD_TOOLS $ENV{BUILD_TOOLS})
endif()
if(BUILD_TOOLS)
    message(STATUS "ED25519: Building test vector generator")

    add_executable(ed25519-gen-testvectors tools/gen_test_vectors.cpp)
    target_link_libraries(ed25519-gen-testvectors ed25519)
endif()

if(BUILD_BENCHMARKS)
    message(STATUS "ED25519: Building benchmarks")

    add_executable(ed25519-benchmark src/benchmark.cpp)
    target_link_libraries(ed25519-benchmark ed25519)
endif()
