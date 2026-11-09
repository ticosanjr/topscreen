#!/usr/bin/env bash
#
# INTEGRATION tests: compile the plugin against the real OBS headers and link
# against a "fake libobs" (tests/harness), allowing creating the filter, running
# frames, firing hotkeys and applying presets without OBS installed.
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="${TOPSCREEN_CHECK_DIR:-${TMPDIR:-/tmp}/topscreen-obscheck}"
CXX="${CXX:-g++}"
OUT_DIR="${ROOT}/build/tests"
OUT="${OUT_DIR}/topscreen_integration"

# Ensures the OBS headers (downloads on first run).
if [ ! -d "$WORK/obs-src/libobs" ]; then
    "$ROOT/tools/check-obs-headers.sh" >/dev/null 2>&1 || true
fi

OBS="$WORK/obs-src"
SIMDE="$WORK/simde-src"
STUB="$WORK/stub"
# OBS 31 moved the frontend API; 30 keeps it under UI/obs-frontend-api.
FRONTEND="$OBS/frontend/api"
[ -d "$FRONTEND" ] || FRONTEND="$OBS/UI/obs-frontend-api"

if [ ! -d "$OBS/libobs" ]; then
    echo "OBS headers missing in $WORK. Run tools/check-obs-headers.sh first."
    exit 2
fi

mkdir -p "$OUT_DIR"

SRC_FILES=$(find "$ROOT/src" -name '*.cpp' ! -name 'plugin-main.cpp' ! -path '*/ui/*' | sort)

echo "Compiling integration ..."
# shellcheck disable=SC2086
"$CXX" -std=c++20 -O1 -g -Wall -Wextra -Wno-missing-field-initializers ${CXXFLAGS:-} \
    -I "$ROOT/src" -I "$ROOT/tests" -I "$ROOT/third_party" -I "$STUB" -I "$OBS/libobs" -I "$FRONTEND" -I "$SIMDE" \
    -DTOPSCREEN_VERSION='"0.0.0-test"' -DTOPSCREEN_INTEGRATION \
    -DTOPSCREEN_TEST_DATA_DIR="\"$ROOT/data\"" \
    "$ROOT/tests/test_main.cpp" \
    "$ROOT"/tests/integration/*.cpp \
    "$ROOT/tests/harness/fake_libobs.cpp" \
    $SRC_FILES \
    -o "$OUT"

echo "Running integration ..."
"$OUT"
