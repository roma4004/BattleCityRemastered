@echo off
rem Two windows: the first brings its own server up and plays on it, the second joins - the usual
rem way to test a match. The server dies with the first window; run-multiplayer-dedicated.bat
rem keeps it standing. Runs build/msbuild/bin/x64/Debug; an argument names another exe.
setlocal
rem BC_ADDRESS overrides this machine's network address. BC_PORT pins the port (set BC_PORT=1234); left alone, the
rem server takes any free one and writes it to server-port.txt, which is where the second window reads it
set "ADDRESS_ARG="
if defined BC_ADDRESS set "ADDRESS_ARG=--address=%BC_ADDRESS%"
if not defined BC_PORT set "BC_PORT=0"
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\..\build\msbuild\bin\x64\Debug\BattleCityRemastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")
if not exist "%GAME_EXE%" (
	echo "%GAME_EXE%" does not exist - build it first 1>&2
	exit /b 1
)

del "%GAME_DIR%server-port.txt" >nul 2>&1
rem --pos places the picture, not the frame - y=40 keeps the title bar on the screen
start "" /D "%GAME_DIR%" "%GAME_EXE%" --server %ADDRESS_ARG% --port=%BC_PORT% --size=800,600 --pos=0,40
rem the first window spawns the server and waits for its port the same way - the file appears once it binds
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
start "" /D "%GAME_DIR%" "%GAME_EXE%" --client --mute %ADDRESS_ARG% --port=%PORT% --size=800,600 --pos=810,40
