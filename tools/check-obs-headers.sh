#!/usr/bin/env bash
#
# Checagem de COMPILACAO do plugin contra os headers REAIS do OBS, sem precisar
# de libobs instalado (usa -fsyntax-only, nao faz link).
#
# Pega erros de API/assinatura/tipos/ordem de campos que os testes do nucleo
# nao pegam. Roda tambem no CI (ver .github/workflows/check-obs-headers.yml).
#
# Uso:
#   tools/check-obs-headers.sh
#
# Variaveis:
#   OBS_REF            referencia do OBS (default: master)
#   TOPSCREEN_CHECK_DIR diretorio de trabalho/cache (default: $TMPDIR/topscreen-obscheck)
#   CXX                compilador (default: g++)
#
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="${TOPSCREEN_CHECK_DIR:-${TMPDIR:-/tmp}/topscreen-obscheck}"
OBS_REF="${OBS_REF:-master}"
CXX="${CXX:-g++}"

mkdir -p "$WORK"
OBS="$WORK/obs-src"
SIMDE="$WORK/simde-src"
STUB="$WORK/stub"

if [ ! -d "$OBS/libobs" ]; then
    echo "Baixando headers do OBS ($OBS_REF) para $WORK ..."
    curl -fsSL --retry 3 -o "$WORK/obs.tar.gz" \
        "https://github.com/obsproject/obs-studio/archive/refs/heads/${OBS_REF}.tar.gz"
    tar xzf "$WORK/obs.tar.gz" -C "$WORK"
    mv "$WORK/obs-studio-${OBS_REF}" "$OBS"
fi

if [ ! -d "$SIMDE/simde" ]; then
    echo "Baixando simde (SIMD do OBS) ..."
    curl -fsSL --retry 3 -o "$WORK/simde.tar.gz" \
        "https://github.com/simd-everywhere/simde/archive/refs/heads/master.tar.gz"
    tar xzf "$WORK/simde.tar.gz" -C "$WORK"
    mv "$WORK/simde-master" "$SIMDE"
fi

# Header gerado pelo CMake do OBS (stub suficiente para a checagem).
mkdir -p "$STUB"
cat > "$STUB/obsconfig.h" <<'EOF'
#pragma once
#define OBS_DATA_PATH "/tmp"
#define OBS_PLUGIN_PATH "/tmp"
#define OBS_PLUGIN_DESTINATION "obs-plugins"
#define OBS_RELEASE_CANDIDATE 0
#define OBS_BETA 0
EOF

INC=(-I"$ROOT/src" -I"$STUB" -I"$OBS/libobs" -I"$SIMDE")
DEF=(-DTOPSCREEN_VERSION='"0.0.0-check"')

if [ -f /usr/include/X11/Xlib.h ]; then
    DEF+=(-DTOPSCREEN_HAVE_X11)
    echo "X11 detectado: backend X11 incluido na checagem."
else
    echo "X11 ausente: backend X11 NAO sera checado."
fi

cd "$ROOT" || exit 2

fail=0
count=0
while IFS= read -r f; do
    case "$f" in
    src/ui/*)
        # O dock exige Qt6 + obs-frontend-api; ver workflow separado.
        continue
        ;;
    esac
    count=$((count + 1))
    # -Wno-missing-field-initializers: os structs do OBS sao inicializados por
    # designated initializers, deixando campos opcionais de fora (intencional).
    if "$CXX" -std=c++20 -fsyntax-only -Wall -Wextra -Wno-missing-field-initializers \
        "${INC[@]}" "${DEF[@]}" "$f"; then
        echo "OK   $f"
    else
        echo "FAIL $f"
        fail=1
    fi
done < <(find src -name '*.cpp' | sort)

echo
if [ "$fail" -eq 0 ]; then
    echo "Checagem OK: $count arquivos compilam contra os headers do OBS."
else
    echo "Checagem FALHOU."
fi
exit "$fail"
