#!/usr/bin/env bash
# Compiles and runs the core test suite (no libobs/cmake).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CXX="${CXX:-g++}"
OUT_DIR="${ROOT}/build/tests"
OUT="${OUT_DIR}/topscreen_tests"

mkdir -p "$OUT_DIR"

echo "Compiling tests with ${CXX} ..."
"${CXX}" -std=c++20 -O1 -g -Wall -Wextra \
    -I "${ROOT}/src" \
    -I "${ROOT}/third_party" \
    -DTOPSCREEN_TEST_DATA_DIR="\"${ROOT}/data\"" \
    -DTOPSCREEN_SOURCE_DIR="\"${ROOT}\"" \
    "${ROOT}"/tests/*.cpp \
    "${ROOT}"/src/core/*.cpp \
    "${ROOT}/src/ts-wallpapers.cpp" \
    -o "${OUT}"

echo "Running tests ..."
"${OUT}"
