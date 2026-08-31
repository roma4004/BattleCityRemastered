# Per-compiler flags and a toolchain capability gate.
#
# Included right after project(), the earliest point where CMAKE_CXX_COMPILER_ID exists. Which
# compiler we got is cmake/SelectCompiler.cmake's job, included before project().
#
# Every warning lives here rather than in CMAKE_CXX_FLAGS_<CFG>_INIT, and for two reasons: those
# variables are read before any compiler is probed, so they cannot branch on one; and being global
# they also reach vendored ThirdParty code, which is added SYSTEM precisely so it stays quiet.
# Optimisation and debug-info flags are left to CMake's own per-compiler defaults - it already knows
# that Debug means -g for GCC and /Zi /Od for MSVC.

include(CheckCXXSourceCompiles)

# Linked PRIVATE by our own targets only.
add_library(project_warnings INTERFACE)

# Coverage instrumentation, opt-in through the Coverage-MinGW preset. It rides on project_warnings
# for the same reason the warnings do: only our four targets link it, so vendored ThirdParty code -
# added SYSTEM - stays uninstrumented and out of the report. -fprofile-abs-path makes the .gcno
# record absolute source paths, so gcovr resolves them from any working directory.
# GCC only: MSYS2's clang 22 ships no libclang_rt.profile.a, so -fprofile-instr-generate fails at link.
option(ENABLE_COVERAGE "Instrument our own targets with gcov" OFF)

if (ENABLE_COVERAGE)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "ENABLE_COVERAGE needs GCC; got ${CMAKE_CXX_COMPILER_ID}.")
    endif ()
    target_compile_options(project_warnings INTERFACE --coverage -fprofile-abs-path)
    target_link_options(project_warnings INTERFACE --coverage)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Understood by both families.
    target_compile_options(project_warnings INTERFACE
            $<$<CONFIG:Debug>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion;-Wsign-conversion;-Wcast-align;-Wunused;-Woverloaded-virtual;-Wnon-virtual-dtor;-Wnull-dereference;-Wdouble-promotion;-Wformat=2>)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC has no -Weverything; this is the curated stand-in. Clang rejects all five outright, which
    # is what forced the split in the first place.
    target_compile_options(project_warnings INTERFACE
            $<$<CONFIG:Debug>:-Wuseless-cast;-Wlogical-op;-Wduplicated-cond;-Wduplicated-branches;-Wnrvo>)

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # The nearest Clang-side equivalents; GCC has no spelling for these. Note -Wunused-private-field
    # and -Wnonportable-include-path are not listed: -Wall already carries them, and both have
    # earned their keep - each found real defects GCC cannot diagnose at all.
    target_compile_options(project_warnings INTERFACE
            $<$<CONFIG:Debug>:-Wextra-semi;-Wimplicit-fallthrough;-Wrange-loop-analysis;-Wshadow-all;-Wdeprecated-copy-with-dtor>)

elseif (MSVC)
    # /W4 is the practical maximum (/Wall drowns in noise from the Windows SDK headers themselves).
    # /Zc:__cplusplus matters more than it looks: without it MSVC reports __cplusplus as 199711L
    # regardless of /std, so any feature check keyed on it silently takes the wrong branch.
    # /utf-8 because our sources carry non-ASCII comments and MSVC otherwise reads them as the
    # system codepage.
    target_compile_options(project_warnings INTERFACE
            /W4 /permissive- /Zc:__cplusplus /Zc:preprocessor /utf-8 /EHsc
            # opt-in warnings closest to the GCC/Clang set above; off by default even at /W4
            $<$<CONFIG:Debug>:/w14242;/w14254;/w14263;/w14265;/w14287;/w14296;/w14311;/w14555;/w14826;/w44062>)
endif ()

# std::expected is the one C++23 feature the project cannot drop - MapLoader::Parse and
# SDL_Config::Init both return it. It is also the sharpest toolchain-age test we have: libstdc++
# gates <expected> behind __cpp_concepts >= 202002L, which Clang only began advertising in 19, so
# Clang 17/18 configure cleanly and then fail in 44 translation units at once. Fail here instead.
check_cxx_source_compiles("
#include <expected>
std::expected<int, int> f() { return 1; }
int main() { return f().value() - 1; }
" HAVE_STD_EXPECTED)

if (NOT HAVE_STD_EXPECTED)
    message(FATAL_ERROR
            "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} does not provide std::expected.\n"
            "Needed: GCC 12+, Clang 19+ against libstdc++ (older Clang reports __cpp_concepts=201907L, "
            "below the 202002L that <expected> demands) or any Clang against libc++ (-stdlib=libc++), "
            "or MSVC 19.36+ with /std:c++23preview. CMake selects that MSVC switch itself from "
            "CMAKE_CXX_STANDARD 23.")
endif ()
