#!/usr/bin/env sh
# The menu path: the first window brings its own server up and plays on it, then a second window joins.
# For a standalone server use run-server-and-clients.sh instead. Optional arg: path to the game exe.
# BC_ADDRESS moves the server off 127.0.0.1. BC_PORT pins the port (BC_PORT=1234); left alone, the
# server takes any free one and writes it to server-port.txt, which is where the second window reads it
host=${BC_ADDRESS:-127.0.0.1}
port=${BC_PORT:-0}
game_exe=${1:-$(dirname "$0")/../build/cmake/Debug-MinGW/BattleCityRemastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")

port_file=server-port.txt
rm -f "$port_file"
"$exe" --server --address="$host" --port="$port" --size=800,600 --pos=0,0 &
first=$!

# the first window spawns the server and waits for its port the same way - the file appears once it binds
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
echo "server on $host:$port"

"$exe" --client --mute --address="$host" --port="$port" --size=800,600 --pos=810,0 &
second=$!

wait "$first" "$second"
