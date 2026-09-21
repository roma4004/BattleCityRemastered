@echo off
rem As run-multiplayer-dedicated.bat, but both windows start with the sound off.
rem Runs build/cmake/Debug-MinGW; an argument names another exe.
setlocal
rem BC_ADDRESS moves the server off 127.0.0.1. BC_PORT pins the port (set BC_PORT=1234); left alone, the
rem server takes any free one and writes it to server-port.txt, which is where the windows read it from
if not defined BC_ADDRESS set "BC_ADDRESS=127.0.0.1"
if not defined BC_PORT set "BC_PORT=0"
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\..\build\cmake\Debug-MinGW\BattleCityRemastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")
if not exist "%GAME_EXE%" (
	echo "%GAME_EXE%" does not exist - build it first 1>&2
	exit /b 1
)

if not exist "%GAME_DIR%BattleCityServer.exe" (
	echo BattleCityServer.exe is not next to the game - build that target as well 1>&2
	exit /b 1
)
del "%GAME_DIR%server-port.txt" >nul 2>&1
rem /MIN - the console shows nothing the log file does not, and unminimised it covers both windows
start "BattleCity server" /MIN /D "%GAME_DIR%" "%GAME_DIR%BattleCityServer.exe" --address=%BC_ADDRESS% --port=%BC_PORT% --port-file=server-port.txt
rem seats go out in connection order, so the listener has to be up before the first window asks - and
rem with a free port the number itself is only known once it is
set "PORT_FILE=%GAME_DIR%server-port.txt"
set "TRIES=0"
:wait_for_port
if exist "%PORT_FILE%" set /p PORT=<"%PORT_FILE%"
if defined PORT goto got_port
set /a TRIES+=1
if %TRIES% GEQ 10 goto no_port
timeout /t 1 /nobreak >nul
goto wait_for_port
:no_port
echo the server never reported a port 1>&2
exit /b 1
:got_port
echo server on %BC_ADDRESS%:%PORT%
start "" /D "%GAME_DIR%" "%GAME_EXE%" --client --mute --address=%BC_ADDRESS% --port=%PORT% --size=800,600 --pos=0,0
timeout /t 1 /nobreak >nul
start "" /D "%GAME_DIR%" "%GAME_EXE%" --client --mute --address=%BC_ADDRESS% --port=%PORT% --size=800,600 --pos=810,0

rem nothing owns the server here the way the game does, so this window sweeps it once the games are gone
:wait_for_windows
timeout /t 2 /nobreak >nul
tasklist /FI "IMAGENAME eq BattleCityRemastered.exe" 2>nul | find /I "BattleCityRemastered.exe" >nul
if not errorlevel 1 goto wait_for_windows
taskkill /IM BattleCityServer.exe /F >nul 2>&1
