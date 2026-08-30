include(${CMAKE_CURRENT_LIST_DIR}/FetchSubmodules.cmake)

# --- SDL3 (+ ttf/image/mixer) --- (per-library git submodules under ThirdParty/SDL3*, same set .sln uses)
set(SDL_SHARED  OFF CACHE BOOL "Build SDL3 as a shared library" FORCE)
set(SDL_STATIC  ON  CACHE BOOL "Build SDL3 as a static library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build the SDL3_test library" FORCE)

set(SDLTTF_VENDORED ON  CACHE BOOL "Use vendored FreeType/HarfBuzz inside SDL3_ttf" FORCE)
set(SDLTTF_SAMPLES  OFF CACHE BOOL "Build SDL3_ttf samples" FORCE)
set(SDLTTF_PLUTOSVG OFF CACHE BOOL "Colour emoji via plutosvg" FORCE) #NOTE: one bitmap font, no emoji
# Pinned OFF, not left to SDL's "off when built as a subproject" default: FetchSubmodules.cmake skips
# their external/* on purpose, so flipping these on would configure against directories we never clone.
set(SDLIMAGE_VENDORED OFF CACHE BOOL "Use vendored codecs inside SDL3_image" FORCE)
set(SDLMIXER_VENDORED OFF CACHE BOOL "Use vendored codecs inside SDL3_mixer" FORCE)

set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
# PNG is the only format we load, and the built-in stb backend decodes it without libpng
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)

set(SDLMIXER_TESTS    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_EXAMPLES OFF CACHE BOOL "" FORCE)
# WAVE stays on - the intro chunk is a .wav; the rest all want a codec out of the external/* we skip
set(SDLMIXER_AIFF    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_AU      OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VOC     OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME     OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD     OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3     OFF CACHE BOOL "" FORCE)
set(SDLMIXER_OPUS    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_STB        OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)

foreach(dir ${SDL3_SUBMODULES})
    add_subdirectory(${dir} SYSTEM)
endforeach()

# The only vendored library that still warns, and SYSTEM cannot reach it: SYSTEM quiets a dependency's
# headers in *our* TUs, while these come from harfbuzz compiling its own .cc, which carries no -W of
# ours. Two sources: hb.hh raises -Wall/-Wextra itself via pragma, and clang defaults add
# -Wnontrivial-memcall.
if (TARGET harfbuzz AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_compile_options(harfbuzz PRIVATE -w)
    # -w does not cover a pragma-set *error*, and hb.hh escalates ~30 diagnostics to errors
    target_compile_definitions(harfbuzz PRIVATE HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR)
endif ()

# --- Boost --- (per-library git submodules under ThirdParty/boost/*, same set .sln uses)
set(BUILD_TESTING OFF) # skip each submodule's own test/ subdirectory

# property_tree hard-requires a Boost::serialization target structurally (unused by our code, ser20
# replaced it) - stub instead of building the real library.
add_library(boost_serialization_stub INTERFACE)
add_library(Boost::serialization ALIAS boost_serialization_stub)

set(BOOST_LIBS
        algorithm align any array asio assert bind compat concept_check config container container_hash
        conversion core describe detail exception function function_types fusion integer intrusive io
        iterator lexical_cast move mp11 mpl multi_index numeric_conversion optional pool predef
        preprocessor property_tree range regex smart_ptr system throw_exception tokenizer tuple
        type_index type_traits typeof unordered utility uuid variant2 winapi)
foreach(lib ${BOOST_LIBS})
    add_subdirectory(ThirdParty/boost/${lib} SYSTEM)
endforeach()

# --- ser20 (replaces Boost.Serialization, C++20 fork of cereal) ---
add_subdirectory(ThirdParty/ser20 SYSTEM)
# ser20 keeps one process-wide static map of class versions and takes a lock around it - but that
# lock compiles to nothing unless this is set, and ser20 only offers its own THREAD_SAFE option
# under if(UNIX). We serialize from the main thread and from the io_context thread at once, so
# without this the map is corrupted by concurrent inserts. Must stay on for every consumer: the
# macro changes the layout of StaticObject::LockGuard.
target_compile_definitions(ser20 PUBLIC SER20_THREAD_SAFE=1)

# --- googletest (shared submodule with .sln/UnitTests.vcxproj) ---
set(BUILD_GMOCK   OFF CACHE BOOL "Build gmock"   FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Install gtest" FORCE)
add_subdirectory(ThirdParty/googletest SYSTEM)
enable_testing()
