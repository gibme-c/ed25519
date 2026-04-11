# libFuzzer harnesses for gibme-c/ed25519.
#
# Opt-in via ED25519_BUILD_FUZZERS=ON at the top level. Clang-only, Linux-only.
# The main `ed25519` target is never touched — library sources are rebuilt into
# private OBJECT libraries with instrumentation so downstream consumers that
# add_subdirectory() the repo with ED25519_BUILD_FUZZERS=ON still get a clean
# `ed25519` target.

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "tests/fuzz requires Clang (libFuzzer + coverage instrumentation)")
endif()

if(WIN32)
    # Windows clang-cl has a libFuzzer + ASan static/dynamic CRT mismatch
    # (`lld-link: /failifmismatch: mismatch detected for 'RuntimeLibrary'`).
    # Fixable by forcing /MT throughout, but out of scope — CI runs fuzzers
    # on Linux clang.
    message(FATAL_ERROR "ED25519_BUILD_FUZZERS is not supported on Windows; use a Linux clang build")
endif()

set(_fuzz_cflags
    -fsanitize=fuzzer-no-link,address,undefined
    -fno-omit-frame-pointer
    -g
    -O1
)
set(_fuzz_linkflags "-fsanitize=fuzzer,address,undefined")

# set_source_files_properties() IFMA/AVX2 flags at the top level are
# directory-scoped and don't reach this subdirectory, so split the per-TU
# sources into separate OBJECT libraries with target-scoped compile options.
set(_ifma_abs "")
set(_avx2_abs "")
if(ED25519_ENABLE_AVX512 AND ED25519_IFMA_SRC)
    foreach(_s IN LISTS ED25519_IFMA_SRC)
        list(APPEND _ifma_abs "${CMAKE_SOURCE_DIR}/${_s}")
    endforeach()
endif()
if(ED25519_ENABLE_AVX2 AND ED25519_AVX2_SRC)
    foreach(_s IN LISTS ED25519_AVX2_SRC)
        list(APPEND _avx2_abs "${CMAKE_SOURCE_DIR}/${_s}")
    endforeach()
endif()

set(_base_abs "")
foreach(_src IN LISTS ED25519_SRC)
    set(_src_abs "${CMAKE_SOURCE_DIR}/${_src}")
    list(FIND _ifma_abs "${_src_abs}" _ifma_idx)
    list(FIND _avx2_abs "${_src_abs}" _avx2_idx)
    if(_ifma_idx EQUAL -1 AND _avx2_idx EQUAL -1)
        list(APPEND _base_abs "${_src_abs}")
    endif()
endforeach()

ed25519_make_object_library(ed25519_fuzz_objs_base "${_base_abs}" ${_fuzz_cflags})
set(_fuzz_obj_targets ed25519_fuzz_objs_base)
if(_ifma_abs)
    ed25519_make_object_library(ed25519_fuzz_objs_ifma "${_ifma_abs}"
        ${_fuzz_cflags} ${ED25519_IFMA_FLAGS})
    list(APPEND _fuzz_obj_targets ed25519_fuzz_objs_ifma)
endif()
if(_avx2_abs)
    ed25519_make_object_library(ed25519_fuzz_objs_avx2 "${_avx2_abs}"
        ${_fuzz_cflags} ${ED25519_AVX2_FLAGS})
    list(APPEND _fuzz_obj_targets ed25519_fuzz_objs_avx2)
endif()

set(FUZZ_TARGETS
    fuzz_fe_roundtrip
    fuzz_ge_frombytes
    fuzz_ristretto255_decode
    fuzz_ristretto255_from_uniform
    fuzz_sc_reduce
    fuzz_sc_check_reduced
    fuzz_x25519
    fuzz_ge_multiscalar_mul
)

set(_fuzz_obj_exprs "")
foreach(_t ${_fuzz_obj_targets})
    list(APPEND _fuzz_obj_exprs "$<TARGET_OBJECTS:${_t}>")
endforeach()

foreach(t ${FUZZ_TARGETS})
    add_executable(ed25519-${t} ${t}.cpp ${_fuzz_obj_exprs})
    # Land the binaries next to ed25519-tests / ed25519-benchmark so CI and
    # local scripts find them at build/ed25519-<name> regardless of where
    # CMake emits add_executable() output by default.
    set_target_properties(ed25519-${t} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    target_include_directories(ed25519-${t} PRIVATE
        $<TARGET_PROPERTY:ed25519,INTERFACE_INCLUDE_DIRECTORIES>
        ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(ed25519-${t} PRIVATE ${_fuzz_cflags})
    set_property(TARGET ed25519-${t} APPEND_STRING PROPERTY LINK_FLAGS " ${_fuzz_linkflags}")
endforeach()

message(STATUS "ED25519: libFuzzer harnesses: ${FUZZ_TARGETS}")
