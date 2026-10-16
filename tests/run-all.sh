#!/usr/bin/env bash
#
# Roda todas as camadas de teste:
#   1. núcleo (unit)          -> ./tests/run.sh
#   2. headers do OBS (build) -> ./tools/check-obs-headers.sh
#   3. integração (fake libobs)-> ./tests/run-integration.sh
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "=============================================="
echo "1/3  Testes do nucleo"
echo "=============================================="
"$ROOT/tests/run.sh"

echo
echo "=============================================="
echo "2/3  Compilacao contra headers do OBS"
echo "=============================================="
"$ROOT/tools/check-obs-headers.sh"

echo
echo "=============================================="
echo "3/3  Testes de integracao (fake libobs)"
echo "=============================================="
"$ROOT/tests/run-integration.sh"

echo
echo "Tudo verde."
