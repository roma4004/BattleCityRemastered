cmake_minimum_required(VERSION 3.15) #NOTE: cmake v3.15+ required by boost from 1.87

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-align -Wunused -Woverloaded-virtual -Wnon-virtual-dtor -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wuseless-cast -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wnrvo") #NOTE: GCC has no single -Weverything switch (Clang-only); this is the closest curated "maximal warnings" set #-fvisibility=hidden
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")

project(BattleCity_remastered)
#TODO: use std::filesystem for resource loading in code
#TODO: create compiler switch msvc, mingw(gcc)
#TODO: migrate to SDL2 to SDL2pp
#TODO: migrate to MSYS2 to install all dependency
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #for Sourcetree
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# --- Global build settings ---
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for all subprojects" FORCE) #NOTE: compat shim for vendored subprojects with an old cmake_minimum_required (e.g. SDL2 itself), unrelated to our own cmake_minimum_required above
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# --- Third-party dependencies (submodule fetch + configure + add_subdirectory) ---
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/ThirdParty) # suppress warnings from vendored headers, own code isn't under this prefix
include(cmake/ConfigureDependencies.cmake)

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
        source/utils/ColliderUtils.cpp
        source/utils/TimeUtils.cpp
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
        gtest gtest_main
        Boost::uuid Boost::asio_core Boost::property_tree
        ser20::ser20
        ${PLATFORM_NETWORK_LIBS}
)

include(GoogleTest)
gtest_discover_tests(unit_tests)

#NOTE: If you are targeting a 64-bit platform, make sure to use x86_64-w64-mingw32 instead of mingw32 for the linker.
target_link_libraries(${PROJECT_NAME}
        SDL2-static SDL2main SDL2_ttf SDL2_image SDL2_mixer
        Boost::uuid Boost::asio_core Boost::property_tree
        ser20::ser20
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
