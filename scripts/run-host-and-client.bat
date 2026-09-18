@echo off
rem The menu path: the first window brings its own server up and plays on it, then a second window joins.
rem For a standalone server use run-server-and-clients.bat instead. Optional arg: path to the game exe.
setlocal
rem BC_ADDRESS moves the server off 127.0.0.1. BC_PORT pins the port (set BC_PORT=1234); left alone, the
rem server takes any free one and writes it to server-port.txt, which is where the second window reads it
if not defined BC_ADDRESS set "BC_ADDRESS=127.0.0.1"
if not defined BC_PORT set "BC_PORT=0"
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\build\cmake\Debug-MinGW\BattleCityRemastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")

del "%GAME_DIR%server-port.txt" >nul 2>&1
start "" /D "%GAME_DIR%" "%GAME_EXE%" --server --address=%BC_ADDRESS% --port=%BC_PORT% --size=800,600 --pos=0,0
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
echo server on %BC_ADDRESS%:%PORT%
start "" /D "%GAME_DIR%" "%GAME_EXE%" --client --mute --address=%BC_ADDRESS% --port=%PORT% --size=800,600 --pos=810,0
