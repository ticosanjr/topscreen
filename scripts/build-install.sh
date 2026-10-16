#!/usr/bin/env bash
#
# Build (e opcionalmente instala) o TopScreen no Linux, detectando o libobs.
#
# Uso:
#   scripts/build-install.sh                 # so compila
#   scripts/build-install.sh --system        # instala no OBS do sistema (sudo)
#   scripts/build-install.sh --prefix DIR    # instala sob DIR (ex.: build do OBS)
#   scripts/build-install.sh --dir DIR       # copia para a pasta de plugins DIR
#   scripts/build-install.sh --dir DIR --data DIR2   # idem, com pasta de dados propria
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
JOBS="$(nproc 2>/dev/null || echo 4)"

MODE="build"
TARGET_PREFIX=""
TARGET_DIR=""
DATA_DIR=""

while [ $# -gt 0 ]; do
	case "$1" in
	--system) MODE="prefix"; TARGET_PREFIX="/usr"; shift ;;
	--prefix) MODE="prefix"; TARGET_PREFIX="${2:?}"; shift 2 ;;
	--dir) MODE="dir"; TARGET_DIR="${2:?}"; shift 2 ;;
	--data) DATA_DIR="${2:?}"; shift 2 ;;
	-j) JOBS="${2:?}"; shift 2 ;;
	-h | --help) sed -n '2,12p' "$0"; exit 0 ;;
	*) echo "opcao desconhecida: $1"; exit 2 ;;
	esac
done

log() { printf '\033[1m%s\033[0m\n' "$*"; }
warn() { printf '\033[33m%s\033[0m\n' "$*"; }
err() { printf '\033[31m%s\033[0m\n' "$*" >&2; }

# ---------------------------------------------------------------------------
# Dependencias
# ---------------------------------------------------------------------------
command -v cmake >/dev/null || {
	err "cmake nao encontrado. Instale: sudo apt install cmake build-essential"
	exit 1
}
command -v g++ >/dev/null || {
	err "g++ nao encontrado. Instale: sudo apt install build-essential"
	exit 1
}

# Headers do libobs
HDR=""
for d in /usr/include/obs /usr/local/include/obs /usr/include; do
	[ -f "$d/obs-module.h" ] && {
		HDR="$d"
		break
	}
done

# Biblioteca do libobs (.so ou .so.0)
LIB=""
for d in /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib /lib/x86_64-linux-gnu; do
	for n in libobs.so libobs.so.0; do
		[ -e "$d/$n" ] && {
			LIB="$d/$n"
			break 2
		}
	done
done

if [ -z "$HDR" ] || [ -z "$LIB" ]; then
	warn "libobs (dev) nao encontrado."
	warn "Instale o pacote de desenvolvimento do OBS e rode de novo."
	warn "  Ubuntu/Debian: sudo apt install libobs-dev"
	warn "  ou adicione o PPA: sudo add-apt-repository ppa:obsproject/obs-studio && sudo apt update"
	warn "  ou passe -DLIBOBS_INCLUDE_DIR/-DLIBOBS_LIBRARY manualmente."
	exit 1
fi

log "libobs: headers=$HDR  lib=$LIB"

# ---------------------------------------------------------------------------
# Configure + build
# ---------------------------------------------------------------------------
log "Configurando ..."
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
	-DLIBOBS_INCLUDE_DIR="$HDR" -DLIBOBS_LIBRARY="$LIB"

log "Compilando (-j$JOBS) ..."
cmake --build "$BUILD" -j"$JOBS"

SO="$BUILD/topscreen.so"
[ -f "$SO" ] || {
	err "topscreen.so nao gerado em $BUILD"
	exit 1
}
log "OK: $SO"

# ---------------------------------------------------------------------------
# Instalacao
# ---------------------------------------------------------------------------
case "$MODE" in
build)
	log "Build concluido. Para instalar:"
	echo "  scripts/build-install.sh --system           # OBS do sistema"
	echo "  scripts/build-install.sh --dir <plugins>    # portable/custom"
	;;
prefix)
	log "Instalando em prefixo $TARGET_PREFIX ..."
	SUDO=""
	[ -w "$TARGET_PREFIX" ] || SUDO="sudo"
	${SUDO} cmake --install "$BUILD" --prefix "$TARGET_PREFIX"
	log "Instalado. Reinicie o OBS."
	;;
dir)
	[ -n "$TARGET_DIR" ] || {
		err "--dir vazio"
		exit 2
	}
	log "Copiando para $TARGET_DIR ..."
	mkdir -p "$TARGET_DIR"
	cp -f "$SO" "$TARGET_DIR/topscreen.so"
	if [ -n "$DATA_DIR" ]; then
		mkdir -p "$DATA_DIR"
		cp -rf "$ROOT/data/." "$DATA_DIR/"
		log "Dados em $DATA_DIR"
	else
		mkdir -p "$TARGET_DIR/topscreen"
		cp -rf "$ROOT/data/." "$TARGET_DIR/topscreen/"
		log "Dados em $TARGET_DIR/topscreen"
	fi
	log "Instalado. Reinicie o OBS."
	;;
esac

cat <<'EOF'

Para ver o log do TopScreen:
  obs --verbose 2>&1 | grep -i topscreen
  # ou abra ~/.config/obs-studio/logs/ (arquivo mais recente)
EOF
