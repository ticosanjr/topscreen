#!/usr/bin/env bash
#
# Testes de INTEGRACAO: compila o plugin contra os headers reais do OBS e linka
# contra um "libobs fake" (tests/harness), permitindo criar o filtro, rodar
# frames, disparar hotkeys e aplicar presets sem o OBS instalado.
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="${TOPSCREEN_CHECK_DIR:-${TMPDIR:-/tmp}/topscreen-obscheck}"
CXX="${CXX:-g++}"
OUT_DIR="${ROOT}/build/tests"
OUT="${OUT_DIR}/topscreen_integration"

# Garante os headers do OBS (baixa na primeira vez).
if [ ! -d "$WORK/obs-src/libobs" ]; then
    "$ROOT/tools/check-obs-headers.sh" >/dev/null 2>&1 || true
fi

OBS="$WORK/obs-src"
SIMDE="$WORK/simde-src"
STUB="$WORK/stub"

if [ ! -d "$OBS/libobs" ]; then
    echo "Headers do OBS ausentes em $WORK. Rode tools/check-obs-headers.sh primeiro."
    exit 2
fi

mkdir -p "$OUT_DIR"

SRC_FILES=$(find "$ROOT/src" -name '*.cpp' ! -name 'plugin-main.cpp' ! -path '*/ui/*' | sort)

echo "Compilando integracao ..."
# shellcheck disable=SC2086
"$CXX" -std=c++20 -O1 -g -Wall -Wextra -Wno-missing-field-initializers \
    -I "$ROOT/src" -I "$ROOT/tests" -I "$ROOT/third_party" -I "$STUB" -I "$OBS/libobs" -I "$SIMDE" \
    -DTOPSCREEN_VERSION='"0.0.0-test"' \
    -DTOPSCREEN_TEST_DATA_DIR="\"$ROOT/data\"" \
    "$ROOT/tests/test_main.cpp" \
    "$ROOT/tests/integration/test_integration.cpp" \
    "$ROOT/tests/harness/fake_libobs.cpp" \
    $SRC_FILES \
    -o "$OUT"

echo "Rodando integracao ..."
"$OUT"
