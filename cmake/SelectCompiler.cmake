# Picks a C++ compiler when the caller did not name one: GCC, else Clang, else MSVC.
#
# Must be included BEFORE project(). After that call CMAKE_CXX_COMPILER is already resolved and
# written to the cache, and assigning it has no effect at all - not even a warning.
#
# Flags for whichever compiler wins are cmake/CompilerSetup.cmake's job, included after project().

# An explicit choice always wins, and every one of these counts as explicit: a preset's
# cacheVariables, -DCMAKE_CXX_COMPILER on the command line, the CXX environment variable, a
# toolchain file, or a cache left by an earlier configure of this same build tree.
if (DEFINED CMAKE_CXX_COMPILER OR DEFINED ENV{CXX} OR CMAKE_TOOLCHAIN_FILE)
    return()
endif ()

# Order is preference, not capability: all three build this project. GCC first because it is what
# the repo is developed against; Clang second because it is the better linter (see CompilerSetup);
# MSVC last because with a Ninja generator it is only reachable from a Developer Command Prompt -
# cl.exe is not on a normal PATH, so the find_program below simply misses it otherwise.
#
# One name per family, kept in parallel variables rather than a list of lists: `foreach(IN LISTS)`
# flattens nested lists, so a "GNU;g++;gcc" element would arrive as three separate iterations.
set(SELECT_COMPILER_ORDER GNU Clang MSVC)
set(SELECT_COMPILER_CXX_GNU g++)
set(SELECT_COMPILER_C_GNU gcc)
set(SELECT_COMPILER_CXX_Clang clang++)
set(SELECT_COMPILER_C_Clang clang)
set(SELECT_COMPILER_CXX_MSVC cl)
set(SELECT_COMPILER_C_MSVC cl)

foreach (family IN LISTS SELECT_COMPILER_ORDER)
    find_program(SELECT_COMPILER_FOUND_CXX_${family} NAMES ${SELECT_COMPILER_CXX_${family}})
    find_program(SELECT_COMPILER_FOUND_C_${family} NAMES ${SELECT_COMPILER_C_${family}})

    if (SELECT_COMPILER_FOUND_CXX_${family} AND SELECT_COMPILER_FOUND_C_${family})
        set(CMAKE_CXX_COMPILER "${SELECT_COMPILER_FOUND_CXX_${family}}")
        set(CMAKE_C_COMPILER "${SELECT_COMPILER_FOUND_C_${family}}")
        message(STATUS "Compiler auto-selected: ${family} - ${CMAKE_CXX_COMPILER}")
        break()
    endif ()
endforeach ()

if (NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR
            "Found none of g++, clang++, cl on PATH. Install one, or name it explicitly with "
            "-DCMAKE_CXX_COMPILER=<path>. For MSVC with a Ninja generator, configure from a "
            "Developer Command Prompt so that cl.exe is on PATH.")
endif ()
