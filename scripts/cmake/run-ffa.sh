#!/usr/bin/env sh
# One window straight into FREE FOR ALL - every tank on its own, the last one standing wins.
# Runs build/cmake/Debug-MinGW; an argument names another exe.
game_exe=${1:-$(dirname "$0")/../../build/cmake/Debug-MinGW/BattleCityRemastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")
if [ ! -f "$exe" ]; then
	echo "$game_exe does not exist - build it first" >&2
	exit 1
fi

"$exe" --ffa
