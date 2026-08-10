cmake_minimum_required(VERSION 3.15) #NOTE: cmake v3.15+ required by boost from 1.87
project(BattleCity_remastered)
#TODO: use std::filesystem for resource loading in code
#TODO: create compiler switch msvc, mingw(gcc)
#TODO: migrate to SDL2 to SDL2pp
#TODO: migrate to MSYS2 to install all dependency
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #for Sourcetree
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Wpedantic -Weverything -Wconversion -Wnrvo -g") #-fvisibility=hidden
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")

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
set(BOOST_URL "https://github.com/boostorg/boost.git")
set(BOOST_VERSION "1.91.0")

# --- Global build/fetch settings ---
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for all subprojects" FORCE) #NOTE: compat shim for vendored subprojects with an old cmake_minimum_required (e.g. SDL2 itself), unrelated to our own cmake_minimum_required above
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(vendored_default FALSE)
set(DEP_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/libraries)
set(FETCHCONTENT_BASE_DIR ${DEP_DOWNLOAD_DIR})

if (DEFINED VCPKG_TARGET_TRIPLET)
    message(STATUS "Using VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
endif ()

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

# --- Boost ---
set(BOOST_INCLUDE_LIBRARIES serialization uuid asio property_tree)
set(BUILD_TESTING OFF) # keep Boost's own ~172 internal test suites out of our build graph

set(BOOST_SUBMODULES
        "tools/cmake
        tools/build
        tools/boost_install
        libs/align
        libs/array
        libs/asio
        libs/assert
        libs/bind
        libs/config
        libs/core
        libs/date_time
        libs/detail
        libs/function
        libs/headers
        libs/integer
        libs/io
        libs/iterator
        libs/move
        libs/mpl
        libs/numeric
        libs/optional
        libs/predef
        libs/preprocessor
        libs/serialization
        libs/smart_ptr
        libs/spirit
        libs/static_assert
        libs/system
        libs/throw_exception
        libs/type_traits
        libs/utility
        libs/winapi
        libs/uuid
        libs/property_tree
        libs/any
        libs/type_index
        libs/container_hash
        libs/multi_index
        libs/tuple
        libs/describe
        libs/mp11
        libs/unordered
        libs/variant
        libs/range
        libs/algorithm
        libs/compat
        libs/concept_check
        libs/context
        libs/conversion
        libs/endian
        libs/exception
        libs/function_types
        libs/fusion
        libs/lexical_cast
        libs/pool
        libs/regex
        libs/variant2
        libs/container
        libs/phoenix
        libs/tokenizer
        libs/typeof
        libs/functional
        libs/intrusive
        libs/proto
        libs/thread
        libs/atomic
        libs/chrono
        libs/ratio"
)

FetchContent_Declare(Boost
        GIT_REPOSITORY ${BOOST_URL}
        GIT_TAG        "boost-${BOOST_VERSION}"
        GIT_PROGRESS   1
        GIT_SHALLOW    TRUE
        GIT_SUBMODULES ${BOOST_SUBMODULES}
)
FetchContent_MakeAvailable(Boost)

# --- googletest ---
FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.11.0
        GIT_PROGRESS   1
        GIT_SHALLOW    TRUE
)

set(BUILD_GMOCK   OFF CACHE BOOL "Build gmock"   FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Install gtest" FORCE)
FetchContent_MakeAvailable(googletest)
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
        source/entities/Obstacles/EagleTile.cpp
        source/entities/Obstacles/BushTile.cpp
        source/entities/Obstacles/IceTile.cpp
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
        source/network/commands/Command.cpp
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
        source/network/commands/Command.cpp
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
        Boost::serialization Boost::uuid Boost::asio Boost::property_tree
        ${PLATFORM_NETWORK_LIBS}
)

include(GoogleTest)
gtest_discover_tests(unit_tests)

#NOTE: If you are targeting a 64-bit platform, make sure to use x86_64-w64-mingw32 instead of mingw32 for the linker.
target_link_libraries(${PROJECT_NAME}
        SDL2-static SDL2main SDL2_ttf SDL2_image SDL2_mixer
        Boost::serialization Boost::uuid Boost::asio Boost::property_tree
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
