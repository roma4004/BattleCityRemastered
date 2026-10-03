#!/usr/bin/env sh
# One window straight into 2P FREE FOR ALL - both keyboard halves in one window, two bots at a time.
# Runs build/cmake/Debug-MinGW; an argument names another exe.
game_exe=${1:-$(dirname "$0")/../../build/cmake/Debug-MinGW/BattleCityRemastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")
if [ ! -f "$exe" ]; then
	echo "$game_exe does not exist - build it first" >&2
	exit 1
fi

"$exe" --2p-ffa
