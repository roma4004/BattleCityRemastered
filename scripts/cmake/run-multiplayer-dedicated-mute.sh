#!/usr/bin/env sh
# As run-multiplayer-dedicated.sh, but both windows start with the sound off.
# Runs build/cmake/Debug-MinGW; an argument names another exe.
# BC_ADDRESS overrides this machine's network address. BC_PORT pins the port (BC_PORT=1234); left alone, the
# server takes any free one and writes it to server-port.txt, which is where the windows read it from
address=${BC_ADDRESS:+--address=$BC_ADDRESS}
port=${BC_PORT:-0}
game_exe=${1:-$(dirname "$0")/../../build/cmake/Debug-MinGW/BattleCityRemastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")
if [ ! -f "$exe" ]; then
	echo "$game_exe does not exist - build it first" >&2
	exit 1
fi

if [ ! -f ./BattleCityServer ] && [ ! -f ./BattleCityServer.exe ]; then
	echo "BattleCityServer is not next to the game - build that target as well" >&2
	exit 1
fi

port_file=server-port.txt
rm -f "$port_file"
# the match the server starts: --seats 1 to 4, --rules classic or ffa, --map a file in Resources/Maps
# without .map, --enemies at once 1 to 4 in classic only; BattleCityServer --help lists every option
# stdin from /dev/null - its console reader would get a background job stopped by the terminal
./BattleCityServer $address --port="$port" --port-file="$port_file" \
	--seats=2 --rules=classic --map=level1 --enemies=4 < /dev/null &
server_pid=$!
# nothing owns the server here the way the game does, so take it down with this script
trap 'kill "$server_pid" 2>/dev/null' EXIT INT TERM

# seats go out in connection order, so the listener has to be up before the first window asks - and
# with a free port the number itself is only known once it is
tries=0
while [ ! -s "$port_file" ] && [ "$tries" -lt 100 ]; do
	sleep 0.05
	tries=$((tries + 1))
done
if [ ! -s "$port_file" ]; then
	echo "the server never reported a port" >&2
	exit 1
fi
port=$(cat "$port_file")
echo "server on ${BC_ADDRESS:-this machine's address}:$port"

# --pos places the picture, not the frame - y=40 keeps the title bar on the screen
"$exe" --client --mute $address --port="$port" --size=800,600 --pos=0,40 &
first=$!
sleep 1
"$exe" --client --mute $address --port="$port" --size=800,600 --pos=810,40 &
second=$!

wait "$first" "$second"
