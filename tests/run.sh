#!/usr/bin/env bash
# Compila e roda a suite de testes do nucleo (sem libobs/cmake).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CXX="${CXX:-g++}"
OUT_DIR="${ROOT}/build/tests"
OUT="${OUT_DIR}/topscreen_tests"

mkdir -p "$OUT_DIR"

echo "Compilando testes com ${CXX} ..."
"${CXX}" -std=c++20 -O1 -g -Wall -Wextra \
    -I "${ROOT}/src" \
    -DTOPSCREEN_TEST_DATA_DIR="\"${ROOT}/data\"" \
    -DTOPSCREEN_SOURCE_DIR="\"${ROOT}\"" \
    "${ROOT}"/tests/*.cpp \
    "${ROOT}"/src/core/*.cpp \
    -o "${OUT}"

echo "Rodando testes ..."
"${OUT}"
