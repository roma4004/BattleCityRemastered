@echo off
rem Dedicated server first, then two client windows side by side. Optional arg: path to the game exe.
setlocal
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\build\cmake\Debug-MinGW\BattleCityRemastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")

rem /MIN - the console shows nothing the log file does not, and unminimised it covers both windows
start "BattleCity server" /MIN /D "%GAME_DIR%" "%GAME_DIR%BattleCityServer.exe"
rem seats go out in connection order, so the listener has to be up before the first window asks
timeout /t 1 /nobreak >nul

start "" /D "%GAME_DIR%" "%GAME_EXE%" client size=800,600 pos=0,0
timeout /t 1 /nobreak >nul
start "" /D "%GAME_DIR%" "%GAME_EXE%" client size=800,600 pos=810,0

rem nothing owns the server here the way a host does, so this window sweeps it once the games are gone
:wait_for_windows
timeout /t 2 /nobreak >nul
tasklist /FI "IMAGENAME eq BattleCityRemastered.exe" 2>nul | find /I "BattleCityRemastered.exe" >nul
if not errorlevel 1 goto wait_for_windows
taskkill /IM BattleCityServer.exe /F >nul 2>&1
