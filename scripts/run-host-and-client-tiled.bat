@echo off
rem Example of pos/size arguments: two 800x600 windows side by side, client without the intro.
setlocal
set "GAME_EXE=%~1"
if "%GAME_EXE%"=="" set "GAME_EXE=%~dp0..\cmake-build-debug-mingw\BattleCity_remastered.exe"
rem /D - assets are copied next to the exe, so the cwd must be its folder
for %%I in ("%GAME_EXE%") do (set "GAME_EXE=%%~fI" & set "GAME_DIR=%%~dpI")
start "" /D "%GAME_DIR%" "%GAME_EXE%" host size=800,600 pos=0,0
start "" /D "%GAME_DIR%" "%GAME_EXE%" client skipintro size=800,600 pos=810,0
