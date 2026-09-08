#!/usr/bin/env sh
# Dedicated server first, then two client windows side by side. Optional arg: path to the game exe.
game_exe=${1:-$(dirname "$0")/../build/cmake/Debug-MinGW/BattleCityRemastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")

./BattleCityServer &
server_pid=$!
# nothing owns the server here the way a host does, so take it down with this script
trap 'kill "$server_pid" 2>/dev/null' EXIT INT TERM
# seats go out in connection order, so the listener has to be up before the first window asks
sleep 1

"$exe" client size=800,600 pos=0,0 &
first=$!
sleep 1
"$exe" client size=800,600 pos=810,0 &
second=$!

wait "$first" "$second"
