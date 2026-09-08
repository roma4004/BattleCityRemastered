cmake_minimum_required(VERSION 3.25) #NOTE: 3.25 for add_subdirectory(<dir> SYSTEM); boost from 1.87 needs 3.15+

# --- Compiler choice: g++ -> clang++ -> MSVC. Must run before project(), which locks the answer in. ---
# Optimisation and debug-info flags are deliberately not set here: CMake's own per-compiler defaults
# already know that Debug means -g for GCC and /Zi /Od for MSVC. Warnings live on the project_warnings
# target in cmake/CompilerSetup.cmake, the only place that can branch on the compiler - this point in
# the file is read before one has been probed.
include(cmake/SelectCompiler.cmake)

project(BattleCityRemastered)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #for Sourcetree
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# --- Per-compiler warnings + toolchain capability gate ---
include(cmake/CompilerSetup.cmake)

# --- Global build settings ---
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for all subprojects" FORCE) #NOTE: compat shim for vendored subprojects with an old cmake_minimum_required, unrelated to our own cmake_minimum_required above
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# --- Third-party dependencies (submodule fetch + configure + add_subdirectory) ---
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/ThirdParty) # suppress warnings from vendored headers, own code isn't under this prefix
include(cmake/ConfigureDependencies.cmake)

# --- Source files ---
include_directories(headers)

# the SDL-facing files, kept apart so GameCore below stays free of SDL
set(GAME_PLATFORM_SOURCES
        source/application/ServerProcess.cpp
        source/application/Game.cpp
        source/application/SDL_Config.cpp
        source/application/UserInput.cpp
        source/components/managers/RenderManager.cpp
        source/components/managers/TextCache.cpp
)

# GameCore sources: every game .cpp except main.cpp (the exe's own entry point) and the platform list above
set(GAME_CORE_SOURCES
        source/application/CommandLineParser.cpp
        source/application/GameConfig.cpp
        source/application/WindowConfig.cpp
        source/application/ProjectConfig.cpp
        source/application/Simulation.cpp
        source/behavior/MoveLikeBulletBeh.cpp
        source/behavior/MoveLikeTankBeh.cpp
        source/behavior/ShootingBeh.cpp
        source/components/AnimatedObjects.cpp
        source/components/BonusSpawner.cpp
        source/components/BulletPool.cpp
        source/components/EventSystem.cpp
        source/components/GameStatistics.cpp
        source/components/LineOfSight.cpp
        source/components/Map.cpp
        source/components/MapLoader.cpp
        source/components/WorldGeometry.cpp
        source/components/Menu.cpp
        source/components/ObstacleSpawner.cpp
        source/components/RightSideBar.cpp
        source/components/LobbyScreen.cpp
        source/components/ScoreBoard.cpp
        source/components/TankPool.cpp
        source/components/TankSpawner.cpp
        source/components/input/InputProviderForMenu.cpp
        source/components/input/InputProviderForBot.cpp
        source/components/input/InputProviderForPlayer.cpp
        source/components/managers/AnimationManager.cpp
        source/components/managers/BonusManager.cpp
        source/components/managers/FramePerSecondManager.cpp
        source/components/managers/GameStateManager.cpp
        source/components/managers/FortressManager.cpp
        source/components/managers/RespawnManager.cpp
        source/components/managers/SpawnManager.cpp
        source/components/managers/TextureManager.cpp
        source/components/managers/WorldScaleManager.cpp
        source/entities/BaseObj.cpp
        source/entities/bonuses/Bonus.cpp
        source/entities/obstacles/BrickWall.cpp
        source/entities/obstacles/BushTile.cpp
        source/entities/obstacles/EagleTile.cpp
        source/entities/obstacles/IceTile.cpp
        source/entities/obstacles/Obstacle.cpp
        source/entities/obstacles/SteelWall.cpp
        source/entities/obstacles/WaterTile.cpp
        source/entities/pawns/Bullet.cpp
        source/entities/pawns/Pawn.cpp
        source/entities/pawns/Tank.cpp
        source/network/Client.cpp
        source/network/ClientNode.cpp
        source/network/FrameChannel.cpp
        source/network/NetworkNodeBase.cpp
        source/network/PeerLink.cpp
        source/network/ReplicationApplier.cpp
        source/network/ReplicationBindings.cpp
        source/network/ReplicationPublisher.cpp
        source/network/Server.cpp
        source/network/Session.cpp
        source/network/ServerNode.cpp
        source/network/Serializer.cpp
        source/utils/ColliderUtils.cpp
        source/utils/Log.cpp
        source/utils/ObjectUtils.cpp
        source/utils/TimeUtils.cpp
        source/utils/Timer.cpp
        source/utils/UuidUtils.cpp
        source/utils/WorldQuery.cpp
)

set(UNIT_TESTS_SOURCES
        UnitTests/AnimationManagerTest.cpp
        UnitTests/BonusTestBasic.cpp
        UnitTests/BonusTestDestroy.cpp
        UnitTests/BonusTestEnemy.cpp
        UnitTests/BotsTest.cpp
        UnitTests/BulletPoolTest.cpp
        UnitTests/MapTest.cpp
        UnitTests/BulletsTestAdvanced.cpp
        UnitTests/BulletsTestBasic.cpp
        UnitTests/CommandLineParserTest.cpp
        UnitTests/CoopBotTest.cpp
        UnitTests/DeadObjectFilterTest.cpp
        UnitTests/DirectionUtilsTest.cpp
        UnitTests/EnemyBotTest.cpp
        UnitTests/GameStateManagerTest.cpp
        UnitTests/GameStateTest.cpp
        UnitTests/ClientMirrorTest.cpp
        UnitTests/TankHealTest.cpp
        UnitTests/TankPoolTest.cpp
        UnitTests/TankTerrainTest.cpp
        UnitTests/MenuInputTest.cpp
        UnitTests/MenuTextBlockTest.cpp
        UnitTests/NetworkTest.cpp
        UnitTests/PauseSyncTest.cpp
        UnitTests/PlayerTest.cpp
        UnitTests/RespawnManagerTest.cpp
        UnitTests/StatisticsTestAdvanced.cpp
        UnitTests/SimulationStepTest.cpp
        UnitTests/StatisticsTestBasic.cpp
        UnitTests/TankSpawnerTest.cpp
        UnitTests/TimeUtilsTest.cpp
        UnitTests/TestUtils.cpp
)

# --- Targets ---
if (WIN32)
    set(PLATFORM_NETWORK_LIBS wsock32 ws2_32) #NOTE: Winsock, needed for boost::asio sockets on Windows; POSIX systems already have sockets in libc
endif ()

# game logic, no SDL: a static lib propagates its deps to consumers (PRIVATE ones too, as LINK_ONLY),
# so anything linked here would reach UnitTests as well
add_library(GameCore STATIC ${GAME_CORE_SOURCES})
target_link_libraries(GameCore PRIVATE project_warnings)
target_link_libraries(GameCore PUBLIC
        Boost::uuid Boost::asio_core Boost::property_tree
        ser20::ser20
        ${PLATFORM_NETWORK_LIBS}
)

#NOTE: without this, include order picks win_thread or posix_thread per TU, and the timer thread is
# started as one and joined as the other - SIGSEGV in ~ClientNode. MSVC has no pthreads at all.
if (WIN32 AND NOT MSVC)
    target_compile_definitions(GameCore PUBLIC BOOST_ASIO_HAS_PTHREADS=1)
endif ()

# window/render/audio/input + the game loop; depends on GameCore, never the other way round
add_library(GamePlatform STATIC ${GAME_PLATFORM_SOURCES})
target_link_libraries(GamePlatform PRIVATE project_warnings)
target_link_libraries(GamePlatform PUBLIC
        GameCore
        SDL3::SDL3-static SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer
)

add_executable(${PROJECT_NAME} source/main.cpp)
#NOTE: GameCore alone, exactly like UnitTests - SDL cannot reach BattleCityServer by
#construction, and the console is all the output it has
add_executable(BattleCityServer source/server_main.cpp)
add_executable(UnitTests ${UNIT_TESTS_SOURCES})

target_include_directories(UnitTests PRIVATE headers)
target_link_libraries(UnitTests PRIVATE project_warnings GameCore gtest gtest_main)

include(GoogleTest)
#NOTE: from the exe folder - the shipped-level test opens Resources/Maps relative to the cwd
gtest_discover_tests(UnitTests WORKING_DIRECTORY $<TARGET_FILE_DIR:UnitTests>)

#NOTE: SDL3 has no SDL2main equivalent - the entry point hijack is header-only in <SDL3/SDL_main.h>
target_link_libraries(${PROJECT_NAME} PRIVATE project_warnings GamePlatform)
target_link_libraries(BattleCityServer PRIVATE project_warnings GameCore)

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
        Resources/Maps/level1.map
        Resources/Sounds/levelStarted.wav
)

#NOTE: a target of its own rather than a post-build step on the game - UnitTests lands in the same
#directory and is regularly built alone, and the test that parses the shipped level needs the map
#to be there. One copy step, one destination, both exes depending on it.
add_custom_target(game_assets)
foreach (asset ${GAME_ASSETS})
    add_custom_command(TARGET game_assets POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/${asset}" #NOTE: absolute - "../" breaks when the build dir moves
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${asset}")
endforeach ()

add_dependencies(${PROJECT_NAME} game_assets)
add_dependencies(UnitTests game_assets)
add_dependencies(BattleCityServer game_assets)

# --- Convenience targets: host + client side by side, no IDE-specific run configuration needed ---
if (WIN32)
    set(RUN_BOTH_EXT bat)
else ()
    set(RUN_BOTH_EXT sh)
endif ()
foreach (variant "" "-skipintro" "-tiled")
    file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/scripts/run-host-and-client${variant}.${RUN_BOTH_EXT} script) #NOTE: cmd.exe reads a forward-slash path as switches
    string(REPLACE "-" "_" suffix "${variant}")
    add_custom_target(run_host_and_client${suffix}
            COMMAND ${script} $<TARGET_FILE:${PROJECT_NAME}>
            DEPENDS ${PROJECT_NAME}
            USES_TERMINAL
            COMMENT "Launching host + client${variant}")
endforeach ()
