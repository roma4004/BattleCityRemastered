@echo off
rem Runs the game twice - host + client. Optional arg: path to the exe.
setlocal
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\cmake-build-debug-mingw\BattleCity_remastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")
start "" /D "%GAME_DIR%" "%GAME_EXE%" host
start "" /D "%GAME_DIR%" "%GAME_EXE%" client
