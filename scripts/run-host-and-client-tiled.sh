#!/usr/bin/env sh
# Example of pos/size arguments: two 800x600 windows side by side, client without the intro.
game_exe=${1:-$(dirname "$0")/../build/cmake/Debug-MinGW/BattleCity_remastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")
"$exe" host size=800,600 pos=0,0 &
"$exe" client skipintro size=800,600 pos=810,0 &
wait
