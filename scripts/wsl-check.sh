#!/usr/bin/env bash
# The Linux check of the Windows working tree, run inside WSL2:
#   wsl.exe -e bash /mnt/c/git/BattleCityRemastered/scripts/wsl-check.sh [--asan] [--tsan] [--coverage] [--game]
# Brings the WSL clone to HEAD plus every uncommitted change, builds the tests, the server and the catalog with GCC
# and runs the tests; each flag adds a tree of its own, --game the game with a window, built and not run. The clone is
# made on the first run at $BC_WSL_CLONE (~/BattleCityRemastered) - building over /mnt/c is too slow.
# From Git Bash, prefix MSYS_NO_PATHCONV=1, or MSYS rewrites the /mnt/c path.
set -u

SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLONE="${BC_WSL_CLONE:-$HOME/BattleCityRemastered}"
JOBS="$(nproc)"
ASAN_SETTINGS="detect_leaks=1:halt_on_error=0:abort_on_error=0:detect_stack_use_after_return=1"
ASAN_SETTINGS+=":strict_init_order=1:check_initialization_order=1"

TREES=(Debug-Linux)
for arg in "$@"; do
	case "$arg" in
		--asan) TREES+=(Asan-Linux) ;;
		--tsan) TREES+=(Tsan-Linux) ;;
		--coverage) TREES+=(Coverage-Linux) ;;
		--game) TREES+=(Game-Linux) ;;
		*) echo "unknown flag $arg - expected --asan, --tsan, --coverage or --game" >&2; exit 2 ;;
	esac
done

# Git for Windows answers in a moment where WSL's git walks /mnt/c for minutes, and knows the tree's line endings
if command -v git.exe > /dev/null; then
	WIN_SRC="$(wslpath -w "$SRC")"
	src_git() { git.exe -C "$WIN_SRC" "$@" | tr -d '\r'; }
else
	src_git() { git -C "$SRC" "$@"; }
fi

if [ ! -d "$CLONE/.git" ]; then
	git clone -q --no-local "file://$SRC" "$CLONE" || exit 1
fi

cd "$CLONE" || exit 1
# what the last run copied in goes; ignored files - the build trees among them - stay
git checkout -q -- . && git clean -fdq
git fetch -q "$SRC" HEAD && git checkout -q --detach FETCH_HEAD || exit 1

mapfile -t changed < <({
	src_git diff --name-only --no-renames --diff-filter=d HEAD
	src_git ls-files --others --exclude-standard
} | sort -u)
mapfile -t deleted < <(src_git diff --name-only --no-renames --diff-filter=D HEAD)
for f in "${changed[@]}"; do
	mkdir -p "$(dirname "$f")"
	cp "$SRC/$f" "$f" || exit 1
	# the Windows tree keeps CRLF in places, the index has LF; a binary file is left as it is
	if grep -Iq . "$f"; then
		sed -i 's/\r$//' "$f"
	fi
done
for f in "${deleted[@]}"; do
	rm -f "$f"
done
echo "HEAD $(git rev-parse --short HEAD) + ${#changed[@]} changed, ${#deleted[@]} deleted"

status=0
for tree in "${TREES[@]}"; do
	echo "== $tree"
	dir="build/cmake/$tree"
	if [ ! -f "$dir/build.ninja" ]; then
		cmake --preset "$tree" > /dev/null || exit 1
	fi

	if ! cmake --build --preset "$tree" -j "$JOBS" > "$dir.build.log" 2>&1; then
		grep -E "error|FAILED" "$dir.build.log" | head -20
		echo "build failed, log in $CLONE/$dir.build.log"
		status=1
		continue
	fi

	#NOTE: the game has no tests of its own - it is run by hand, under WSLg
	if [ "$tree" = Game-Linux ]; then
		echo "built: cd $CLONE/$dir && ./BattleCityRemastered"
		continue
	fi

	log="$CLONE/$dir.test.log"
	case "$tree" in
		Asan-Linux)
			(cd "$dir" && ASAN_OPTIONS="$ASAN_SETTINGS" UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0 \
				LSAN_OPTIONS=report_objects=0 ./UnitTests --gtest_brief=1 > "$log" 2>&1) ;;
		Tsan-Linux)
			(cd "$dir" && TSAN_OPTIONS=halt_on_error=0:second_deadlock_stack=1:history_size=7 \
				./UnitTests --gtest_brief=1 > "$log" 2>&1) ;;
		Coverage-Linux)
			find "$dir" -name '*.gcda' -delete
			(cd "$dir" && ./UnitTests --gtest_brief=1 > "$log" 2>&1) ;;
		*)
			(cd "$dir" && ./UnitTests --gtest_brief=1 > "$log" 2>&1) ;;
	esac
	tests=$?

	grep -E "FAILED|tests from .* ran|PASSED" "$log"
	# a leak or a race does not show in the exit code - the sanitizer's own summary does
	if grep -qE "^SUMMARY: |runtime error:|WARNING: ThreadSanitizer" "$log"; then
		grep -E "^SUMMARY: |runtime error:" "$log" | sort | uniq -c | head -20
		status=1
	fi
	[ "$tests" -eq 0 ] || status=1
	echo "log in $log"

	if [ "$tree" = Coverage-Linux ]; then
		gcovr="$(command -v gcovr || echo "$HOME/.venvs/gcovr/bin/gcovr")"
		if [ -x "$gcovr" ]; then
			(cd "$dir" && "$gcovr" --root ../../.. --filter '.*/source/.*' \
				--gcov-ignore-parse-errors negative_hits.warn_once_per_file --txt coverage.txt \
				--print-summary CMakeFiles/GameCore.dir | tail -3)
		else
			echo "no gcovr - the counters are in $dir, a report needs gcovr (pip install gcovr)"
		fi
	fi
done

exit "$status"
