#NOTE: to build boost from source, run from boots source folder
#(example for MinGW, if you using MSVC replace "--toolset=gcc" to "--toolset=msvc")
# and replace {path_to_project_source} with your project path to install boost --prefix={path_to_project_source}
# .\bootstrap.bat gcc
# then run this for build only required boost component:
# ./b2 --toolset=gcc --build-type=complete link=static runtime-link=shared --prefix={path_to_project_source}/libraries/boost_gcc install
# or run this for build all boost component:
# ./b2 --toolset=gcc --build-type=complete link=static runtime-link=shared --prefix={path_to_project_source}/libraries/boost_gcc install --with-serialization

cmake_minimum_required(VERSION 3.28.6)
#cmake_minimum_required(VERSION 3.11...3.15)
project(BattleCity_remastered)
#TODO: create compiler switch msvc, mingw(gcc)
#TODO: migrate to SDL2 to SDL2pp
#TODO: migrate to MSYS2 to install all dependency
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Wpedantic -Weverything -Wconversion -Wnrvo -g") #-fvisibility=hidden
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")

include(FetchContent)
set(vendored_default FALSE)
set(SDL2_URL "https://github.com/libsdl-org/SDL.git")
set(SDL2_TAG "release-2.30.8")
set(SDL2_TTF_URL "https://github.com/libsdl-org/SDL_ttf.git")
set(SDL2_TTF_TAG "release-2.22.0")
set(SDL2_IMG_URL "https://github.com/libsdl-org/SDL_image.git")
set(SDL2_IMG_TAG "release-2.8.2")
set(SDL2_MXR_URL "https://github.com/libsdl-org/SDL_mixer.git")
set(SDL2_MXR_TAG "release-2.8.0")
set(BOOST_URL "https://github.com/boostorg/boost.git")
set(BOOST_VERSION "1.85.0")
string(REPLACE "." "_" BOOST_VERSION_UNDERSCORE "${BOOST_VERSION}")
set(BOOST_TAG "boost-${BOOST_VERSION}")
set(SDL2_MIXER_DOWNLOAD_URL "https://github.com/libsdl-org/SDL_mixer.git")

cmake_policy(SET CMP0148 NEW)
include (FindPython)
find_package(Python REQUIRED COMPONENTS Interpreter Development)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Build shared SDL2 libraries")
set(SDL_STATIC ON CACHE BOOL "Build static SDL2 libraries")
set(SDL_TTF_SHARED OFF CACHE BOOL "Build shared SDL2_ttf library")
set(SDL_TTF_STATIC ON CACHE BOOL "Build static SDL2_ttf library")
option(SDL2_TTF_STATIC "Build static SDL2_ttf library" ON)

set(SDL2MIXER_FLUIDSYNTH OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_LIBXMP OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MODPLUG OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OGG OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MP3 OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_TREMOR OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)

set(DEP_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/libraries)
set(FETCHCONTENT_BASE_DIR ${DEP_DOWNLOAD_DIR})

if(DEFINED VCPKG_TARGET_TRIPLET)
    message(STATUS "Using VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
endif()

FetchContent_Declare(SDL2 GIT_REPOSITORY ${SDL2_URL} GIT_TAG ${SDL2_TAG} GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE #[[OVERRIDE_FIND_PACKAGE]])
FetchContent_Declare(SDL2_ttf GIT_REPOSITORY ${SDL2_TTF_URL} GIT_TAG ${SDL2_TTF_TAG} GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE)
FetchContent_Declare(SDL2_image GIT_REPOSITORY ${SDL2_IMG_URL} GIT_TAG ${SDL2_IMG_TAG} GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE #[[OVERRIDE_FIND_PACKAGE]])
FetchContent_Declare(SDL2_mixer GIT_REPOSITORY ${SDL2_MXR_URL} GIT_TAG ${SDL2_MXR_TAG} GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE #[[OVERRIDE_FIND_PACKAGE]])

FetchContent_MakeAvailable(SDL2)
FetchContent_GetProperties(SDL2)

set(FREETYPE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/libraries/sdl2_ttf-src/external/freetype/include")
set(FREETYPE_LIBRARY "${CMAKE_SOURCE_DIR}/libraries/sdl2_ttf-src/external/freetype/libfreetype.a")
FetchContent_MakeAvailable(SDL2_ttf)
add_subdirectory(${CMAKE_SOURCE_DIR}/libraries/sdl2_ttf-src/external/freetype EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/libraries/sdl2_ttf-src/external/freetype)
FetchContent_GetProperties(SDL2_ttf)

FetchContent_MakeAvailable(SDL2_image)
FetchContent_GetProperties(SDL2_image)

set(libxmp_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/libraries/sdl2_mixer-src/external/libxmp/include")
set(libxmp_LIBRARY "${CMAKE_SOURCE_DIR}/libraries/sdl2_mixer-src/external/libxmp/libxmp.a")
FetchContent_MakeAvailable(SDL2_mixer)
add_subdirectory(${CMAKE_SOURCE_DIR}/libraries/sdl2_mixer-src/external/libxmp EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/libraries/sdl2_mixer-src/external/libxmp)
FetchContent_GetProperties(SDL2_mixer)

#FetchContent_MakeAvailable(SDL2 SDL2_ttf SDL2_image #[[SDL2_mixer]])

#FetchContent_Declare(boost
##        GIT_REPOSITORY ""
##        GIT_TAG "boost-1.85.0"
##        GIT_SUBMODULES_RECURSE ON
#        URL https://github.com/boostorg/boost/archive/refs/tags/boost-1.85.0.zip
#        SOURCE_DIR ${Boost_EXTRACTION_DIRECTORY}
#)
#set(BOOST_ROOT                  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/boost_gcc/)
##FetchContent_MakeAvailable(Boost)
#FetchContent_GetProperties(boost)
#if(NOT boost_POPULATED)
#    FetchContent_Populate(Boost)
##    set(BOOST_ROOT ${Boost_SOURCE_DIR})
##    set(Boost_LIBRARY_DIRS ${Boost_SOURCE_DIR}/libs)
##    set(Boost_NO_SYSTEM_PATHS TRUE)
##    set(Boost_USE_STATIC_LIBS ON)
#    set(boost_INCLUDE_DIR ${boost_SOURCE_DIR})
#    execute_process(COMMAND git submodule update --init WORKING_DIRECTORY
#            find_package(boost #[[1.85.0]]#[[ COMPONENTS Serialization]] REQUIRED)
##    message(FATAL_ERROR ${Boost_INCLUDE_DIR})
##    message(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}") # log the
#    if(Boost_FOUND)
##        include_directories(${Boost_SOURCE_DIR}/libs/serialization/include)
##        include_directories(${Boost_SOURCE_DIR}/libs/asio/include)
#    endif()
#endif()
cmake_policy(SET CMP0144 NEW)
include(ExternalProject)

include(ProcessorCount)
ProcessorCount(ThreadAmount)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(BOOST_SEMAPHORE ${DEP_DOWNLOAD_DIR}/checklist_install_done)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Using Release build of Boost in Debug configuration")
    set(Boost_USE_DEBUG_LIBS ON)
    set(Boost_DEBUG ON)
#    set(BOOST_VARIANT debug)
#    set(BOOST_SEMAPHORE ${DEP_DOWNLOAD_DIR}/checklist_debug_install_done)
else()
    set(Boost_USE_DEBUG_LIBS OFF)
#    set(BOOST_VARIANT release)
#    set(BOOST_SEMAPHORE ${DEP_DOWNLOAD_DIR}/checklist_release_install_done)
endif()
#set(boost_configure_command ./bootstrap.sh gcc)
#set(boost_build_command ./b2.exe --toolset=gcc link=static --build-type=complete runtime-link=shared variant=release --prefix=<INSTALL_DIR> --with-serialization -j8 stage install)
set(BOOST_SUBMODULES
    "tools/build
    tools/boost_install
    libs/headers
    libs/asio
    libs/assert
    libs/config
    libs/core
    libs/mpl
    libs/array
    libs/smart_ptr
    libs/bind
    libs/move
    libs/detail
    libs/io
    libs/iterator
    libs/preprocessor
    libs/static_assert
    libs/throw_exception
    libs/type_traits
    libs/serialization
    libs/spirit
    libs/integer
    libs/predef
    libs/optional
    libs/function
    libs/utility
    libs/align
    libs/system
    libs/winapi
    libs/date_time
    libs/numeric")

if (NOT EXISTS ${BOOST_SEMAPHORE})
    ExternalProject_Add(Boost
            GIT_REPOSITORY ${BOOST_URL}
            GIT_TAG ${BOOST_TAG}
            GIT_PROGRESS 1
            GIT_SHALLOW 1
            GIT_SHALLOW TRUE
#            UPDATE_DISCONNECTED TRUE
            GIT_SUBMODULES ${BOOST_SUBMODULES} # load only specified submodules instead of all
            PREFIX boost
            DOWNLOAD_DIR ${DEP_DOWNLOAD_DIR}
            SOURCE_DIR ${DEP_DOWNLOAD_DIR}/boost_${BOOST_VERSION_UNDERSCORE}
#            BINARY_DIR ${DEP_DOWNLOAD_DIR}/boost_${BOOST_VERSION_UNDERSCORE}_build
            INSTALL_DIR ${DEP_DOWNLOAD_DIR}/boost_install
#            TMP_DIR Boost/tmp
#            STAMP_DIR Boost/stamp

#            LOG_DOWNLOAD ON
#            LOG_CONFIGURE ON
#            LOG_BUILD ON
#            LOG_INSTALL ON

#            PATCH_COMMAND ./bootstrap.sh gcc
            CONFIGURE_COMMAND ./bootstrap.sh gcc --prefix=<INSTALL_DIR> --with-libraries=serialization
#            CONFIGURE_COMMAND ""#./bootstrap.sh gcc --prefix=<INSTALL_DIR> --with-libraries=serialization
            BUILD_COMMAND ./b2 --prefix=<INSTALL_DIR> --toolset=gcc link=static threading=multi --build-type=complete runtime-link=static #[[variant=${BOOST_VARIANT}]] address-model=64 --with-serialization -j${ThreadAmount} -d+2 stage install
            BUILD_IN_SOURCE 1
            INSTALL_COMMAND ""
            DOWNLOAD_EXTRACT_TIMESTAMP true
            BUILD_ALWAYS TRUE
    )

    ExternalProject_Add_Step(Boost create_semaphore
            COMMAND ${CMAKE_COMMAND} -E touch ${BOOST_SEMAPHORE}
            DEPENDEES install
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    message(STATUS "--------------------------------------------------")
    message(STATUS "Boost: is already installed, so sipping this step.")
    message(STATUS "--------------------------------------------------")
endif()

set(Boost_NO_SYSTEM_PATHS ON)
set(BOOST_ROOT ${CMAKE_SOURCE_DIR}/libraries/boost_install)
find_package(Boost ${BOOST_VERSION} CONFIG COMPONENTS serialization)
if(Boost_FOUND)
    message(STATUS "------------------------------------------------------------")
    message(STATUS "Found Boost ${Boost_VERSION} with libraries: ${Boost_LIBRARIES}")
    message(STATUS "include directory: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "------------------------------------------------------------")
else ()
    message(STATUS "------------------------------------------------------------")
    message(STATUS "Could not find Boost, for the first time ignore it, will be downloaded as dependency during build run.")
    message(STATUS "If you see ninja: error: loading 'build.ninja': The system cannot find the file specified,")
    message(STATUS "just reload cmake project to fix it, then run the build")
    message(STATUS "------------------------------------------------------------")
endif()

include_directories(headers)

add_executable(${PROJECT_NAME}
        source/behavior/MoveLikeAIBeh.cpp
        source/behavior/MoveLikeBulletBeh.cpp
        source/behavior/MoveLikeTankBeh.cpp
        source/behavior/ShootingBeh.cpp
        source/bonuses/Bonus.cpp
        source/bonuses/BonusGrenade.cpp
        source/bonuses/BonusHelmet.cpp
        source/bonuses/BonusShovel.cpp
        source/bonuses/BonusStar.cpp
        source/bonuses/BonusSystem.cpp
        source/bonuses/BonusTank.cpp
        source/bonuses/BonusTimer.cpp
        source/BulletPool.cpp
        source/GameFailure.cpp
        source/GameStatistics.cpp
        source/GameSuccess.cpp
        source/ConfigFailure.cpp
        source/ConfigSuccess.cpp
        source/input/InputProviderForMenu.cpp
        source/input/InputProviderForPlayerOne.cpp
        source/input/InputProviderForPlayerOneNet.cpp
        source/input/InputProviderForPlayerTwo.cpp
        source/input/InputProviderForPlayerTwoNet.cpp
        source/LineOfSight.cpp
        source/main.cpp
        source/BaseObj.cpp
        source/Map.cpp
        source/Menu.cpp
        source/obstacles/Brick.cpp
        source/obstacles/FortressObstacle.cpp
        source/obstacles/Iron.cpp
        source/obstacles/Water.cpp
        source/pawns/Bullet.cpp
        source/pawns/CoopAI.cpp
        source/pawns/Enemy.cpp
        source/pawns/Pawn.cpp
        source/pawns/PlayerOne.cpp
        source/pawns/PlayerTwo.cpp
        source/pawns/Tank.cpp
        source/ObjRectangle.cpp
        source/SDLEnvironment.cpp
        source/Server.cpp
        source/TankSpawner.cpp
        source/utils/ColliderUtils.cpp
        source/utils/TimeUtils.cpp
)

if (NOT EXISTS ${BOOST_SEMAPHORE})
    add_dependencies(${PROJECT_NAME} Boost)
endif()

#NOTE: If you are targeting a 64-bit platform, make sure to use x86_64-w64-mingw32 instead of mingw32 for the linker.
target_link_libraries(${PROJECT_NAME}
        SDL2-static
        SDL2main
        SDL2_ttf
        SDL2_image
        SDL2_mixer
        ${Boost_LIBRARIES}
        wsock32  #NOTE: this for windows socket
        ws2_32   #NOTE: this for windows socket
)

set(FONT_PATH                   Resources/Fonts)
set(FONT_PATH_NAME              ${FONT_PATH}/bc7x7.ttf)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD            # Adds a post-build event to ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different           # which executes "cmake - E copy_if_different..."
        "../${FONT_PATH_NAME}"                                  # <--this is in-file
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${FONT_PATH_NAME}") # <--this is out-file path

set(LOGO_PATH                   Resources/Images)
set(LOGO_PATH_NAME              ${LOGO_PATH}/Title.png)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "../${LOGO_PATH_NAME}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${LOGO_PATH_NAME}")

set(LEVEL_STARTED_PATH                   Resources/Sounds)
set(LEVEL_STARTED_PATH_NAME              ${LEVEL_STARTED_PATH}/levelStarted.wav)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "../${LEVEL_STARTED_PATH_NAME}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${LEVEL_STARTED_PATH_NAME}")
