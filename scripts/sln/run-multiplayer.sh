#!/usr/bin/env sh
# Two windows: the first brings its own server up and plays on it, the second joins - the usual
# way to test a match. The server dies with the first window; run-multiplayer-dedicated.sh
# keeps it standing. Runs build/msbuild/bin/x64/Debug; an argument names another exe.
# BC_ADDRESS overrides this machine's network address. BC_PORT pins the port (BC_PORT=1234); left alone, the
# server takes any free one and writes it to server-port.txt, which is where the second window reads it
address=${BC_ADDRESS:+--address=$BC_ADDRESS}
port=${BC_PORT:-0}
game_exe=${1:-$(dirname "$0")/../../build/msbuild/bin/x64/Debug/BattleCityRemastered}
# assets are copied next to the exe, so the cwd must be its folder
cd "$(dirname "$game_exe")" || exit 1
exe=./$(basename "$game_exe")
if [ ! -f "$exe" ]; then
	echo "$game_exe does not exist - build it first" >&2
	exit 1
fi

port_file=server-port.txt
rm -f "$port_file"
# --pos places the picture, not the frame - y=40 keeps the title bar on the screen
"$exe" --server $address --port="$port" --size=800,600 --pos=0,40 &
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
echo "server on ${BC_ADDRESS:-this machine's address}:$port"

"$exe" --client --mute $address --port="$port" --size=800,600 --pos=810,40 &
second=$!

wait "$first" "$second"
