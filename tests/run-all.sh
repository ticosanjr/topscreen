#!/usr/bin/env bash
#
# Runs all test layers:
#   1. core (unit)            -> ./tests/run.sh
#   2. OBS headers (build)    -> ./tools/check-obs-headers.sh
#   3. integration (fake libobs)-> ./tests/run-integration.sh
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "=============================================="
echo "1/3  Core tests"
echo "=============================================="
"$ROOT/tests/run.sh"

echo
echo "=============================================="
echo "2/3  Compilation against OBS headers"
echo "=============================================="
"$ROOT/tools/check-obs-headers.sh"

echo
echo "=============================================="
echo "3/3  Integration tests (fake libobs)"
echo "=============================================="
"$ROOT/tests/run-integration.sh"

echo
echo "All green."
