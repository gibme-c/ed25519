# Empirical constant-time (dudect) harnesses.
# Opt-in via ED25519_BUILD_CT_TESTS=ON at the top level.
#
# Platform: Linux / macOS / other POSIX only — upstream dudect.h uses
# <unistd.h>, <fcntl.h>, /dev/urandom, and sleep(), which are not available
# on MSVC Windows builds. The parent CMakeLists.txt gates add_subdirectory
# on a non-Windows check.

set(CT_HARNESSES
    ct_ge_scalarmult
    ct_ge_scalarmult_base
)

foreach(harness ${CT_HARNESSES})
    add_executable(ed25519-${harness} ${harness}.cpp)
    target_include_directories(ed25519-${harness} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ed25519-${harness} PRIVATE ed25519)
    # Math library for sqrt/pow/fabs used by dudect.
    if(UNIX AND NOT APPLE)
        target_link_libraries(ed25519-${harness} PRIVATE m)
    endif()
    ed25519_apply_exe_flags(ed25519-${harness})
endforeach()
