cmake_minimum_required(VERSION 2.8.12)

if(DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: Debug, Release, RelWithDebInfo")
else()
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug, Release, RelWithDebInfo")
endif()

message(STATUS "Mode: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo Release CACHE STRING INTERNAL)
set(ARCH native CACHE STRING  "CPU to build for: -march value or native")

message(STATUS "Building for target architecture: ${ARCH}")

set(LIB_MAJOR_VERSION "1")
set(LIB_MINOR_VERSION "0")
set(LIB_PATCH_VERSION "1")
set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

if(CMAKE_VERSION VERSION_LESS 3.0)
    project(ed25519 CXX)
    enable_language(CXX)
else()
    cmake_policy(SET CMP0048 NEW)
    project(ed25519 VERSION "${LIB_VERSION_STRING}" LANGUAGES CXX)
endif()

if(NOT MSVC)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache package... Activating...")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    endif()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "ED25519: Activating 64-bit enhancements...")
    add_definitions(-D__USE_64BIT__)
endif()

## This section describes our general CMake setup options
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SKIP_INSTALL_RULES OFF FORCE)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON FORCE)
set(CMAKE_SUPPRESS_REGENERATION ON)
set(CMAKE_POLICY_WARNING_CMP0048 OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Enable c++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## We only build static binaries -- this is left here for our dependencies
set(STATIC ON CACHE BOOL FORCE "Link libraries statically? Forced to ON")
add_definitions(-DSTATICLIB)

option(BUILD_BENCHMARK "Build benchmark testing tool" OFF)
if(DEFINED ENV{BUILD_BENCHMARK})
    set(BUILD_BENCHMARK $ENV{BUILD_BENCHMARK})
    message(STATUS "ED25519: Activating benchmark testing tool")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++17 /DWIN32_LEAN_AND_MEAN /wd4267 /wd4804 /D_DLL")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    if("${ARCH}" STREQUAL "default")
        set(ARCH_FLAG "")
    else()
        set(ARCH_FLAG "-march=${ARCH}")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wuninitialized")

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
    src/donna128_scalarmult.cpp
    src/equal.cpp
    src/fe_0.cpp
    src/fe_1.cpp
    src/fe_add.cpp
    src/fe_cmov.cpp
    src/fe_copy.cpp
    src/fe_divpowm1.cpp
    src/fe_frombytes.cpp
    src/fe_invert.cpp
    src/fe_isnegative.cpp
    src/fe_isnonzero.cpp
    src/fe_mul.cpp
    src/fe_neg.cpp
    src/fe_pow22523.cpp
    src/fe_sq.cpp
    src/fe_sq2.cpp
    src/fe_sub.cpp
    src/fe_tobytes.cpp
    src/ge_add.cpp
    src/ge_cached_0.cpp
    src/ge_cached_cmov.cpp
    src/ge_check_subgroup_precomp_negate_vartime.cpp
    src/ge_double_scalarmult_negate_vartime.cpp
    src/ge_double_scalarmult_base_negate_vartime.cpp
    src/ge_dsm_precomp.cpp
    src/ge_frombytes_negate_vartime.cpp
    src/ge_fromfe_frombytes_negate_vartime.cpp
    src/ge_madd.cpp
    src/ge_msub.cpp
    src/ge_mul8.cpp
    src/ge_p1p1_to_p2.cpp
    src/ge_p1p1_to_p3.cpp
    src/ge_p2_0.cpp
    src/ge_p2_dbl.cpp
    src/ge_p2_to_p3.cpp
    src/ge_p3_0.cpp
    src/ge_p3_dbl.cpp
    src/ge_p3_to_cached.cpp
    src/ge_p3_to_p2.cpp
    src/ge_p3_tobytes.cpp
    src/ge_precomp_0.cpp
    src/ge_precomp_cmov.cpp
    src/ge_scalarmult.cpp
    src/ge_scalarmult_base.cpp
    src/ge_sub.cpp
    src/ge_tobytes.cpp
    src/load_3.cpp
    src/load_4.cpp
    src/negative.cpp
    src/ref10_scalarmult.cpp
    src/sc_0.cpp
    src/sc_add.cpp
    src/sc_check.cpp
    src/sc_isnonzero.cpp
    src/sc_mul.cpp
    src/sc_muladd.cpp
    src/sc_mulsub.cpp
    src/sc_reduce.cpp
    src/sc_reduce32.cpp
    src/sc_sub.cpp
    src/signum.cpp
    src/slide.cpp
)

add_library(ed25519 STATIC ${ED25519_SRC})
target_include_directories(ed25519 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BUILD_BENCHMARK)
    set(BENCHMARK_SOURCES src/benchmark.cpp)

    add_executable(benchmark-ed25519 ${BENCHMARK_SOURCES})
    target_link_libraries(benchmark-ed25519 ed25519)
    set_property(TARGET benchmark-ed25519 PROPERTY OUTPUT_NAME "benchmark")
endif()
