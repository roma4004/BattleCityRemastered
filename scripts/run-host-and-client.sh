#!/usr/bin/env sh
# The menu path: the host window starts its own server and joins it, then a second window connects.
# For a standalone server use run-server-and-clients.sh instead. Optional arg: path to the game exe.
game_exe=${1:-$(dirname "$0")/../build/cmake/Debug-MinGW/BattleCityRemastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")

"$exe" host size=800,600 pos=0,0 &
host=$!
# the host has to spawn its server and bind the public port before the second window dials it
sleep 2
"$exe" client skipintro size=800,600 pos=810,0 &
client=$!

wait "$host" "$client"
