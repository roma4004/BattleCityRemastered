@echo off
rem One window straight into 2P FREE FOR ALL - both keyboard halves in one window, two bots at a time.
rem Runs build/msbuild/bin/x64/Debug; an argument names another exe.
setlocal
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\..\build\msbuild\bin\x64\Debug\BattleCityRemastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")
if not exist "%GAME_EXE%" (
	echo "%GAME_EXE%" does not exist - build it first 1>&2
	exit /b 1
)

start "" /D "%GAME_DIR%" "%GAME_EXE%" --2p-ffa
