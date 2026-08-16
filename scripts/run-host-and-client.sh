#!/usr/bin/env sh
# Runs the game twice - host + client. Optional arg: path to the exe.
game_exe=${1:-$(dirname "$0")/../cmake-build-debug-mingw/BattleCity_remastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")
"$exe" host &
"$exe" client &
wait
