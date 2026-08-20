@echo off
rem Runs the game twice - host + client. Optional arg: path to the exe.
setlocal
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\build\cmake\Debug-MinGW\BattleCity_remastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")
start "" /D "%GAME_DIR%" "%GAME_EXE%" host size=800,600
start "" /D "%GAME_DIR%" "%GAME_EXE%" client size=800,600
