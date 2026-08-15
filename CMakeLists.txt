cmake_minimum_required(VERSION 3.15) #NOTE: cmake v3.15+ required by boost from 1.87

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Wpedantic -Weverything -Wconversion -Wnrvo -g") #-fvisibility=hidden
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")

project(BattleCity_remastered)
#TODO: use std::filesystem for resource loading in code
#TODO: create compiler switch msvc, mingw(gcc)
#TODO: migrate to SDL2 to SDL2pp
#TODO: migrate to MSYS2 to install all dependency
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #for Sourcetree
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# --- Dependency versions ---
include(FetchContent)
set(SDL2_URL "https://github.com/libsdl-org/SDL.git")
set(SDL2_TAG "release-2.32.10")
set(SDL2_TTF_URL "https://github.com/libsdl-org/SDL_ttf.git")
set(SDL2_TTF_TAG "release-2.24.0")
set(SDL2_IMG_URL "https://github.com/libsdl-org/SDL_image.git")
set(SDL2_IMG_TAG "release-2.8.12")
set(SDL2_MXR_URL "https://github.com/libsdl-org/SDL_mixer.git")
set(SDL2_MXR_TAG "release-2.8.2")

# --- Global build/fetch settings ---
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for all subprojects" FORCE) #NOTE: compat shim for vendored subprojects with an old cmake_minimum_required (e.g. SDL2 itself), unrelated to our own cmake_minimum_required above
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(vendored_default FALSE)
set(DEP_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/libraries)
set(FETCHCONTENT_BASE_DIR ${DEP_DOWNLOAD_DIR})

# --- SDL2 (+ ttf/image/mixer) ---
set(SDL2TTF_VENDORED ON CACHE BOOL "Use vendored FreeType/HarfBuzz inside SDL2_ttf" FORCE)

set(SDL2MIXER_MIDI    OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MP3     OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_FLAC    OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MOD     OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OGG     OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OPUS    OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_VORBIS  OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL2       GIT_REPOSITORY ${SDL2_URL}     GIT_TAG ${SDL2_TAG}     GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE)
FetchContent_Declare(SDL2_ttf   GIT_REPOSITORY ${SDL2_TTF_URL} GIT_TAG ${SDL2_TTF_TAG} GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE)
FetchContent_Declare(SDL2_image GIT_REPOSITORY ${SDL2_IMG_URL} GIT_TAG ${SDL2_IMG_TAG} GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE)
FetchContent_Declare(SDL2_mixer GIT_REPOSITORY ${SDL2_MXR_URL} GIT_TAG ${SDL2_MXR_TAG} GIT_SUBMODULES_RECURSE ON GIT_PROGRESS 1 GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(SDL2 SDL2_ttf SDL2_image SDL2_mixer)

# --- Boost --- (per-library git submodules under ThirdParty/boost/*, same set .sln uses)
set(BUILD_TESTING OFF) # skip each submodule's own test/ subdirectory

# property_tree hard-requires a Boost::serialization target structurally (unused by our code, ser20
# replaced it) - stub instead of building the real library.
add_library(boost_serialization_stub INTERFACE)
add_library(Boost::serialization ALIAS boost_serialization_stub)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ThirdParty/boost/uuid/CMakeLists.txt")
    find_package(Git QUIET)
    if(NOT Git_FOUND)
        message(FATAL_ERROR "ThirdParty/boost submodules are not initialized and git was not found. Run: git submodule update --init --recursive")
    endif()
    message(STATUS "ThirdParty/boost submodules are not initialized, fetching them now...")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ThirdParty/boost
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE BOOST_SUBMODULE_RESULT
    )
    if(NOT BOOST_SUBMODULE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to fetch ThirdParty/boost submodules. Run manually: git submodule update --init --recursive")
    endif()
endif()

set(BOOST_LIBS
        algorithm align any array asio assert bind compat concept_check config container container_hash
        conversion core describe detail exception function function_types fusion integer intrusive io
        iterator lexical_cast move mp11 mpl multi_index numeric_conversion optional pool predef
        preprocessor property_tree range regex smart_ptr system throw_exception tokenizer tuple
        type_index type_traits typeof unordered utility uuid variant2 winapi)
foreach(lib ${BOOST_LIBS})
    add_subdirectory(ThirdParty/boost/${lib})
endforeach()

# --- ser20 (replaces Boost.Serialization, C++20 fork of cereal) ---
add_subdirectory(ThirdParty/ser20)

# --- googletest (shared submodule with .sln/UnitTests.vcxproj) ---
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ThirdParty/googletest/CMakeLists.txt")
    find_package(Git QUIET)
    if(Git_FOUND)
        message(STATUS "ThirdParty/googletest submodule is not initialized, fetching it now...")
        execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ThirdParty/googletest
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE GTEST_SUBMODULE_RESULT
        )
        if(NOT GTEST_SUBMODULE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to fetch ThirdParty/googletest submodule. Run manually: git submodule update --init --recursive")
        endif()
    else()
        message(FATAL_ERROR "ThirdParty/googletest submodule is not initialized and git was not found. Run: git submodule update --init --recursive")
    endif()
endif()

set(BUILD_GMOCK   OFF CACHE BOOL "Build gmock"   FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Install gtest" FORCE)
add_subdirectory(ThirdParty/googletest)
enable_testing()

# --- Source files ---
include_directories(headers)

set(SOURCE_FILES
        source/application/ConfigFailure.cpp
        source/application/ConfigSuccess.cpp
        source/application/GameConfig.cpp
        source/application/GameFailure.cpp
        source/application/GameSuccess.cpp
        source/application/SDL_Config.cpp
        source/application/UserInput.cpp
        source/behavior/MoveLikeBulletBeh.cpp
        source/behavior/MoveLikeTankBeh.cpp
        source/behavior/ShootingBeh.cpp
        source/components/AnimatedObjects.cpp
        source/components/BonusSpawner.cpp
        source/components/BulletPool.cpp
        source/components/GameStatistics.cpp
        source/components/LineOfSight.cpp
        source/components/Map.cpp
        source/components/Menu.cpp
        source/components/ObstacleSpawner.cpp
        source/components/RightSideBar.cpp
        source/components/ScoreBoard.cpp
        source/components/TankSpawner.cpp
        source/components/input/InputProviderForMenu.cpp
        source/components/input/InputProviderForPlayerOne.cpp
        source/components/input/InputProviderForPlayerOneNet.cpp
        source/components/input/InputProviderForPlayerTwo.cpp
        source/components/input/InputProviderForPlayerTwoNet.cpp
        source/components/managers/AnimationManager.cpp
        source/components/managers/BonusEffectManager.cpp
        source/components/managers/FramePerSecondManager.cpp
        source/components/managers/RenderManager.cpp
        source/components/managers/RespawnManager.cpp
        source/components/managers/SpawnManager.cpp
        source/components/managers/DelayedSpawnManager.cpp
        source/components/managers/GameStateManager.cpp
        source/components/managers/TextureManager.cpp
        source/entities/BaseObj.cpp
        source/entities/obstacles/EagleTile.cpp
        source/entities/obstacles/BushTile.cpp
        source/entities/obstacles/IceTile.cpp
        source/entities/bonuses/Bonus.cpp
        source/entities/bonuses/BonusCaliber.cpp
        source/entities/bonuses/BonusGrenade.cpp
        source/entities/bonuses/BonusHelmet.cpp
        source/entities/bonuses/BonusShovel.cpp
        source/entities/bonuses/BonusStar.cpp
        source/entities/bonuses/BonusTank.cpp
        source/entities/bonuses/BonusTimer.cpp
        source/entities/obstacles/BrickWall.cpp
        source/entities/obstacles/FortressWall.cpp
        source/entities/obstacles/Obstacle.cpp
        source/entities/obstacles/SteelWall.cpp
        source/entities/obstacles/WaterTile.cpp
        source/entities/pawns/Bot.cpp
        source/entities/pawns/Bullet.cpp
        source/entities/pawns/CoopBot.cpp
        source/entities/pawns/Enemy.cpp
        source/entities/pawns/Pawn.cpp
        source/entities/pawns/Player.cpp
        source/entities/pawns/Tank.cpp
        source/main.cpp
        source/network/Client.cpp
        source/network/ClientHandler.cpp
        source/network/Server.cpp
        source/network/ServerHandler.cpp
        source/network/commands/BonusStatus.cpp
        source/network/commands/GameStateChange.cpp
        source/network/commands/SignalEvent.cpp
        source/network/commands/TankSpawnComplete.cpp
        source/network/commands/BonusDeSpawn.cpp
        source/network/commands/BonusSpawn.cpp
        source/network/commands/CommandBatch.cpp
        source/network/commands/Dispose.cpp
        source/network/commands/FortressChange.cpp
        source/network/commands/HealthChange.cpp
        source/network/commands/KeyStateChange.cpp
        source/network/commands/ObstacleSpawn.cpp
        source/network/commands/PositionChange.cpp
        source/network/commands/RespawnTank.cpp
        source/network/commands/StatisticsChange.cpp
        source/network/commands/TankShot.cpp
        source/utils/ColliderUtils.cpp
        source/utils/Logger.cpp
        source/utils/NetworkLogger.cpp
        source/utils/Timer.cpp
        source/utils/TimeUtils.cpp
        source/utils/UuidUtils.cpp
)

set(SOURCE_FILES_FOR_UNIT_TESTS
        source/application/GameConfig.cpp
        source/Utils/ColliderUtils.cpp
        source/Utils/TimeUtils.cpp
        source/behavior/MoveLikeBulletBeh.cpp
        source/behavior/MoveLikeTankBeh.cpp
        source/behavior/ShootingBeh.cpp
        source/components/BonusSpawner.cpp
        source/components/BulletPool.cpp
        source/components/GameStatistics.cpp
        source/components/LineOfSight.cpp
        source/components/ObstacleSpawner.cpp
        source/components/TankSpawner.cpp
        source/components/input/InputProviderForMenu.cpp
        source/components/input/InputProviderForPlayerOne.cpp
        source/components/input/InputProviderForPlayerOneNet.cpp
        source/components/input/InputProviderForPlayerTwo.cpp
        source/components/input/InputProviderForPlayerTwoNet.cpp
        source/components/managers/BonusEffectManager.cpp
        source/components/managers/RespawnManager.cpp
        source/components/managers/DelayedSpawnManager.cpp
        source/components/managers/GameStateManager.cpp
        source/components/Map.cpp
        source/entities/BaseObj.cpp
        source/entities/bonuses/Bonus.cpp
        source/entities/bonuses/BonusCaliber.cpp
        source/entities/bonuses/BonusGrenade.cpp
        source/entities/bonuses/BonusHelmet.cpp
        source/entities/bonuses/BonusShovel.cpp
        source/entities/bonuses/BonusStar.cpp
        source/entities/bonuses/BonusTank.cpp
        source/entities/bonuses/BonusTimer.cpp
        source/entities/obstacles/BrickWall.cpp
        source/entities/obstacles/EagleTile.cpp
        source/entities/obstacles/FortressWall.cpp
        source/entities/obstacles/BushTile.cpp
        source/entities/obstacles/IceTile.cpp
        source/entities/obstacles/Obstacle.cpp
        source/entities/obstacles/SteelWall.cpp
        source/entities/obstacles/WaterTile.cpp
        source/entities/pawns/Bot.cpp
        source/entities/pawns/Bullet.cpp
        source/entities/pawns/CoopBot.cpp
        source/entities/pawns/Enemy.cpp
        source/entities/pawns/Pawn.cpp
        source/entities/pawns/Player.cpp
        source/entities/pawns/Tank.cpp
        source/network/Client.cpp
        source/network/ClientHandler.cpp
        source/network/Server.cpp
        source/network/ServerHandler.cpp
        source/network/commands/BonusStatus.cpp
        source/network/commands/GameStateChange.cpp
        source/network/commands/SignalEvent.cpp
        source/network/commands/TankSpawnComplete.cpp
        source/network/commands/BonusDeSpawn.cpp
        source/network/commands/BonusSpawn.cpp
        source/network/commands/CommandBatch.cpp
        source/network/commands/Dispose.cpp
        source/network/commands/FortressChange.cpp
        source/network/commands/HealthChange.cpp
        source/network/commands/KeyStateChange.cpp
        source/network/commands/ObstacleSpawn.cpp
        source/network/commands/PositionChange.cpp
        source/network/commands/RespawnTank.cpp
        source/network/commands/StatisticsChange.cpp
        source/network/commands/TankShot.cpp
        source/utils/Logger.cpp
        source/utils/NetworkLogger.cpp
        source/utils/Timer.cpp
        source/utils/UuidUtils.cpp
)

set(UNIT_TESTS_FILES
        UnitTests/BonusTestBasic.cpp
        UnitTests/BonusTestDestroy.cpp
        UnitTests/BonusTestEnemy.cpp
        UnitTests/BotsTest.cpp
        UnitTests/CoopBotTest.cpp
        UnitTests/BulletsTestAdvanced.cpp
        UnitTests/BulletsTestBasic.cpp
        UnitTests/EnemyBotTest.cpp
        UnitTests/NetworkTest.cpp
        UnitTests/PlayerTest.cpp
        UnitTests/RespawnManagerTest.cpp
        UnitTests/TankSpawnerTest.cpp
        UnitTests/GameStateManagerTest.cpp
        UnitTests/StatisticsTestAdvanced.cpp
        UnitTests/StatisticsTestBasic.cpp
        UnitTests/TestUtils.cpp
)

# --- Targets ---
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_executable(unit_tests ${SOURCE_FILES_FOR_UNIT_TESTS} ${UNIT_TESTS_FILES})

if (WIN32)
    set(PLATFORM_NETWORK_LIBS wsock32 ws2_32) #NOTE: Winsock, needed for boost::asio sockets on Windows; POSIX systems already have sockets in libc
endif ()

target_include_directories(unit_tests PRIVATE headers)
target_link_libraries(unit_tests
        gtest
        gtest_main
        ser20::ser20 Boost::uuid Boost::asio_core Boost::property_tree
        ${PLATFORM_NETWORK_LIBS}
)

include(GoogleTest)
gtest_discover_tests(unit_tests)

#NOTE: If you are targeting a 64-bit platform, make sure to use x86_64-w64-mingw32 instead of mingw32 for the linker.
target_link_libraries(${PROJECT_NAME}
        SDL2-static SDL2main SDL2_ttf SDL2_image SDL2_mixer
        ser20::ser20 Boost::uuid Boost::asio_core Boost::property_tree
        ${PLATFORM_NETWORK_LIBS}
)

# --- Game assets (copied next to the exe post-build) ---
set(GAME_ASSETS
        Resources/Fonts/PressStart2P-vaV7.ttf
        Resources/Images/Title.png
        Resources/Images/SpriteSheet.png
        Resources/Images/menuSelectorP1.png
        Resources/Images/PS5_Buttons/Create.png
        Resources/Images/PS5_Buttons/Cross.png
        Resources/Images/PS5_Buttons/D-Pad.png
        Resources/Images/PS5_Buttons/Home.png
        Resources/Images/PS5_Buttons/Options.png
        Resources/Images/PS5_Buttons/Triangle.png
        Resources/Images/Xbox_Buttons/D-Pad.png
        Resources/Images/Xbox_Buttons/Home.png
        Resources/Images/Xbox_Buttons/Menu.png
        Resources/Images/Xbox_Buttons/View.png
        Resources/Images/Xbox_Buttons/A.png
        Resources/Images/Xbox_Buttons/Y.png
        Resources/Sounds/levelStarted.wav
)

foreach (asset ${GAME_ASSETS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "../${asset}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${asset}")
endforeach ()
