cmake_minimum_required(VERSION 3.27)
project(BattleCity_remastered)

#if(WIN32)
#    set(
#            CMAKE_EXE_LINKER_FLAGS
#            "${CMAKE_EXE_LINKER_FLAGS} -Wl,--subsystem,windows"
#    )
#endif()

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -pedantic -Everything")
#set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall")

set(SDL2_DIR                    ${CMAKE_SOURCE_DIR}/SDL2-2.30.0/cmake)
set(SDL2_image_DIR              ${CMAKE_SOURCE_DIR}/SDL2_image-2.8.2/cmake)
set(SDL2_ttf_DIR                ${CMAKE_SOURCE_DIR}/SDL2_ttf-2.22.0/cmake)
set(SDL2_mixer_DIR              ${CMAKE_SOURCE_DIR}/SDL2_mixer-2.8.0/cmake)

set(SDL2_PATH                   ${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.30.0)
set(SDL2_Image_PATH             ${CMAKE_CURRENT_SOURCE_DIR}/SDL2_image-2.8.2)
set(SDL2_TTF_PATH               ${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-2.22.0)
set(SDL2_Mixer_PATH             ${CMAKE_CURRENT_SOURCE_DIR}/SDL2_mixer-2.8.0)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${SDL2_LIBRARY}/cmake/sdl2-config.cmake")
find_package(SDL2 REQUIRED)
## 1. Look for a SDL2 package, 2. look for the SDL2 component and 3. fail if none can be found
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
## 1. Look for a SDL2 package, 2. Look for the SDL2main component and 3. DO NOT fail when SDL2main is not available
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
find_package(SDL2_image REQUIRED)
find_package(SDL2_image REQUIRED CONFIG REQUIRED COMPONENTS SDL2_image)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_ttf REQUIRED CONFIG REQUIRED COMPONENTS SDL2_ttf)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_mixer REQUIRED CONFIG REQUIRED COMPONENTS SDL2_mixer)

# Support both 32 and 64 bit builds
if (!${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    # 64 bits
    set(SDL2_BIN                ${SDL2_PATH}/x86_64-w64-mingw32/bin)
    set(SDL2_INCLUDE            ${SDL2_PATH}/x86_64-w64-mingw32/include/SDL2)
    set(SDL2_LIBRARY            ${SDL2_PATH}/x86_64-w64-mingw32/lib)

    set(SDL2_Image_BIN          ${SDL2_Image_PATH}/x86_64-w64-mingw32/bin)
    set(SDL2_Image_INCLUDE      ${SDL2_Image_PATH}/x86_64-w64-mingw32/include/SDL2)
    set(SDL2_Image_LIBRARY      ${SDL2_Image_PATH}/x86_64-w64-mingw32/lib)

    set(SDL2_TTF_BIN            ${SDL2_TTF_PATH}/x86_64-w64-mingw32/bin)
    set(SDL2_TTF_INCLUDE        ${SDL2_TTF_PATH}/x86_64-w64-mingw32/include/SDL2)
    set(SDL2_TTF_LIBRARY        ${SDL2_TTF_PATH}/x86_64-w64-mingw32/lib)

    set(SDL2_Mixer_BIN          ${SDL2_Mixer_PATH}/x86_64-w64-mingw32/bin)
    set(SDL2_Mixer_INCLUDE      ${SDL2_Mixer_PATH}/x86_64-w64-mingw32/include/SDL2)
    set(SDL2_Mixer_LIBRARY      ${SDL2_Mixer_PATH}/x86_64-w64-mingw32/lib)
else ()
    # 32 bits
    set(SDL2_BIN                ${SDL2_PATH}/i686-w64-mingw32/bin)
    set(SDL2_INCLUDE            ${SDL2_PATH}/i686-w64-mingw32/include/SDL2)
    set(SDL2_LIBRARY            ${SDL2_PATH}/i686-w64-mingw32/lib)

    set(SDL2_Image_BIN          ${SDL2_Image_PATH}/i686-w64-mingw32/bin)
    set(SDL2_Image_INCLUDE      ${SDL2_Image_PATH}/i686-w64-mingw32/include/SDL2)
    set(SDL2_Image_LIBRARY      ${SDL2_Image_PATH}/i686-w64-mingw32/lib)

    set(SDL2_TTF_BIN            ${SDL2_TTF_PATH}/i686-w64-mingw32/bin)
    set(SDL2_TTF_INCLUDE        ${SDL2_TTF_PATH}/i686-w64-mingw32/include/SDL2)
    set(SDL2_TTF_LIBRARY        ${SDL2_TTF_PATH}/i686-w64-mingw32/lib)

    set(SDL2_Mixer_BIN          ${SDL2_Mixer_PATH}/i686-w64-mingw32/bin)
    set(SDL2_Mixer_INCLUDE      ${SDL2_Mixer_PATH}/i686-w64-mingw32/include/SDL2)
    set(SDL2_Mixer_LIBRARY      ${SDL2_Mixer_PATH}/i686-w64-mingw32/lib)
endif ()

include_directories(${SDL2_INCLUDE})
include_directories(${SDL2_Image_INCLUDE})
include_directories(${SDL2_TTF_INCLUDE})
include_directories(${SDL2_Mixer_INCLUDE})

include_directories(headers)

#add_executable(${PROJECT_NAME} WIN32 main.cpp )
add_executable(${PROJECT_NAME}
        source/main.cpp
        source/BaseObj.cpp
        source/Pawn.cpp
        source/PlayerOne.cpp
        source/PlayerTwo.cpp
        source/Bullet.cpp
        source/Brick.cpp
        source/Config.cpp
        source/ConfigFailure.cpp
        source/ConfigSuccess.cpp
        source/Game.cpp
        source/Iron.cpp
        source/SDLEnvironment.cpp
        source/Water.cpp
        source/Map.cpp
        source/Rectangle.cpp
)

#If you are targeting a 64-bit platform, make sure to use x86_64-w64-mingw32 instead of mingw32 for the linker.
target_link_libraries(
        ${PROJECT_NAME}
        mingw32
        SDL2main
        SDL2
        SDL2_image
        SDL2_ttf
        SDL2_mixer
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different       # which executes "cmake - E copy_if_different..."
        "${SDL2_BIN}/SDL2.dll"                              # <--this is in-file
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)                 # <--this is out-file path

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different       # which executes "cmake - E copy_if_different..."
        "${SDL2_Image_BIN}/SDL2_image.dll"                  # <--this is in-file
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)                 # <--this is out-file path

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different       # which executes "cmake - E copy_if_different..."
        "${SDL2_TTF_BIN}/SDL2_ttf.dll"                      # <--this is in-file
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)                 # <--this is out-file path

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different       # which executes "cmake - E copy_if_different..."
        "${SDL2_Mixer_BIN}/SDL2_mixer.dll"                  # <--this is in-file
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)                 # <--this is out-file path