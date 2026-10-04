@echo off
rem A server process of its own, then two client windows - it outlives them, so a window can leave
rem and come back. run-multiplayer.bat is the shorter path when that does not matter.
rem Runs build/cmake/Debug-MinGW; an argument names another exe.
setlocal
rem BC_ADDRESS overrides this machine's network address. BC_PORT pins the port (set BC_PORT=1234); left alone, the
rem server takes any free one and writes it to server-port.txt, which is where the windows read it from
set "ADDRESS_ARG="
if defined BC_ADDRESS set "ADDRESS_ARG=--address=%BC_ADDRESS%"
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
rem the match the server starts: --seats 1 to 4, --rules classic or ffa, --map a file in Resources/Maps
rem without .map, --enemies at once 1 to 4 in classic only; BattleCityServer --help lists every option
rem --bots fill the seats nobody took, up to one fewer than the seats; --start=now starts with
rem whoever is in, full waits for every seat, and a player joining later takes over a bot
rem /MIN - the console shows nothing the log file does not, and unminimized it covers both windows
start "BattleCity server" /MIN /D "%GAME_DIR%" "%GAME_DIR%BattleCityServer.exe" %ADDRESS_ARG% --port=%BC_PORT% ^
	--port-file=server-port.txt --seats=2 --rules=classic --map=level1 --enemies=4 --bots=0 --start=full
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
if defined BC_ADDRESS (echo server on %BC_ADDRESS%:%PORT%) else echo server on this machine's address:%PORT%
rem --pos places the picture, not the frame - y=40 keeps the title bar on the screen
start "" /D "%GAME_DIR%" "%GAME_EXE%" --client %ADDRESS_ARG% --port=%PORT% --size=800,600 --pos=0,40
timeout /t 1 /nobreak >nul
start "" /D "%GAME_DIR%" "%GAME_EXE%" --client %ADDRESS_ARG% --port=%PORT% --size=800,600 --pos=810,40

rem nothing owns the server here the way the game does, so this window sweeps it once the games are gone
:wait_for_windows
timeout /t 2 /nobreak >nul
tasklist /FI "IMAGENAME eq BattleCityRemastered.exe" 2>nul | find /I "BattleCityRemastered.exe" >nul
if not errorlevel 1 goto wait_for_windows
taskkill /IM BattleCityServer.exe /F >nul 2>&1
