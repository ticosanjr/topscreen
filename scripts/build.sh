#!/usr/bin/env bash
#
# Builds TopScreen (plugin + tests) and runs the core tests.
#
# Usage:
#   scripts/build.sh                 # plugin + tests, and runs the tests
#   scripts/build.sh --tests-only    # tests only (no libobs needed)
#   scripts/build.sh --no-test       # build only
#   scripts/build.sh -- <cmake args> # extra arguments for cmake
#
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${TOPSCREEN_BUILD_DIR:-$ROOT/build}"
JOBS="$(nproc 2>/dev/null || echo 4)"

TESTS_ONLY=0
RUN_TESTS=1
EXTRA=()

while [ $# -gt 0 ]; do
	case "$1" in
	--tests-only) TESTS_ONLY=1; shift ;;
	--no-test) RUN_TESTS=0; shift ;;
	--) shift; EXTRA=("$@"); break ;;
	-h | --help) sed -n '2,10p' "$0"; exit 0 ;;
	*) EXTRA+=("$1"); shift ;;
	esac
done

log() { printf '\033[1m%s\033[0m\n' "$*"; }
err() { printf '\033[31m%s\033[0m\n' "$*" >&2; }

command -v cmake >/dev/null || {
	err "cmake not found. Install: sudo apt install cmake build-essential"
	exit 1
}

PLUGIN=ON
[ "$TESTS_ONLY" = 1 ] && PLUGIN=OFF

log "Configuring (plugin=$PLUGIN) ..."
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
	-DTOPSCREEN_BUILD_PLUGIN="$PLUGIN" -DTOPSCREEN_BUILD_TESTS=ON "${EXTRA[@]}" || exit $?

log "Building (-j$JOBS) ..."
cmake --build "$BUILD" -j"$JOBS" || exit $?

if [ "$RUN_TESTS" = 1 ]; then
	log "Running tests ..."
	ctest --test-dir "$BUILD" --output-on-failure || exit $?
fi

log "Build complete (build: $BUILD)"
