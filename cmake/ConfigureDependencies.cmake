include(${CMAKE_CURRENT_LIST_DIR}/FetchSubmodules.cmake)

# --- SDL2 (+ ttf/image/mixer) --- (per-library git submodules under ThirdParty/SDL2*, same set .sln uses)
set(SDL2TTF_VENDORED ON CACHE BOOL "Use vendored FreeType/HarfBuzz inside SDL2_ttf" FORCE)
# Pinned OFF, not left to SDL's "off when built as a subproject" default: FetchSubmodules.cmake skips
# their external/* on purpose, so flipping these on would configure against directories we never clone.
set(SDL2IMAGE_VENDORED OFF CACHE BOOL "Use vendored codecs inside SDL2_image" FORCE)
set(SDL2MIXER_VENDORED OFF CACHE BOOL "Use vendored codecs inside SDL2_mixer" FORCE)

set(SDL2MIXER_MIDI    OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MP3     OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_FLAC    OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MOD     OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OGG     OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OPUS    OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_VORBIS  OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)

foreach(dir ${SDL2_SUBMODULES})
    add_subdirectory(${dir} SYSTEM)
endforeach()

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

# --- googletest (shared submodule with .sln/UnitTests.vcxproj) ---
set(BUILD_GMOCK   OFF CACHE BOOL "Build gmock"   FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Install gtest" FORCE)
add_subdirectory(ThirdParty/googletest SYSTEM)
enable_testing()
