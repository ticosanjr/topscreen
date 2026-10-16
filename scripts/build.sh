#!/usr/bin/env bash
#
# Compila o TopScreen (plugin + testes) e roda os testes do nucleo.
#
# Uso:
#   scripts/build.sh                 # plugin + testes, e roda os testes
#   scripts/build.sh --tests-only    # so os testes (nao precisa de libobs)
#   scripts/build.sh --no-test       # so compila
#   scripts/build.sh -- <args cmake> # argumentos extras para o cmake
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
	err "cmake nao encontrado. Instale: sudo apt install cmake build-essential"
	exit 1
}

PLUGIN=ON
[ "$TESTS_ONLY" = 1 ] && PLUGIN=OFF

log "Configurando (plugin=$PLUGIN) ..."
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
	-DTOPSCREEN_BUILD_PLUGIN="$PLUGIN" -DTOPSCREEN_BUILD_TESTS=ON "${EXTRA[@]}" || exit $?

log "Compilando (-j$JOBS) ..."
cmake --build "$BUILD" -j"$JOBS" || exit $?

if [ "$RUN_TESTS" = 1 ]; then
	log "Rodando testes ..."
	ctest --test-dir "$BUILD" --output-on-failure || exit $?
fi

log "Build concluido (build: $BUILD)"
