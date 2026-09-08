@echo off
rem The menu path: the host window starts its own server and joins it, then a second window connects.
rem For a standalone server use run-server-and-clients.bat instead. Optional arg: path to the game exe.
setlocal
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\build\cmake\Debug-MinGW\BattleCityRemastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")

start "" /D "%GAME_DIR%" "%GAME_EXE%" host size=800,600 pos=0,0
rem the host has to spawn its server and bind the public port before the second window dials it
timeout /t 2 /nobreak >nul
start "" /D "%GAME_DIR%" "%GAME_EXE%" client skipintro size=800,600 pos=810,0
