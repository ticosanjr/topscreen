#!/usr/bin/env bash
#
# Build (e opcionalmente instala) o TopScreen no Linux, detectando o libobs.
#
# Uso:
#   scripts/build-install.sh                 # so compila
#   scripts/build-install.sh --diagnose      # mostra o ambiente (nao compila)
#   scripts/build-install.sh --system        # instala no OBS do sistema (sudo)
#   scripts/build-install.sh --prefix DIR    # instala sob DIR (ex.: build do OBS)
#   scripts/build-install.sh --dir DIR       # copia para a pasta de plugins DIR
#   scripts/build-install.sh --pause         # espera Enter no final (duplo-clique)
#
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
JOBS="$(nproc 2>/dev/null || echo 4)"

MODE="build"
TARGET_PREFIX=""
TARGET_DIR=""
DATA_DIR=""
PAUSE=0
DIAGNOSE=0

while [ $# -gt 0 ]; do
	case "$1" in
	--system) MODE="system"; shift ;;
	--user) MODE="user"; shift ;;
	--prefix) MODE="prefix"; TARGET_PREFIX="${2:?}"; shift 2 ;;
	--dir) MODE="dir"; TARGET_DIR="${2:?}"; shift 2 ;;
	--data) DATA_DIR="${2:?}"; shift 2 ;;
	--pause) PAUSE=1; shift ;;
	--diagnose) DIAGNOSE=1; shift ;;
	-j) JOBS="${2:?}"; shift 2 ;;
	-h | --help) sed -n '2,13p' "$0"; exit 0 ;;
	*) echo "opcao desconhecida: $1"; exit 2 ;;
	esac
done

# Estilo desligado se nao for terminal.
if [ -t 1 ]; then
	B=$'\033[1m'; Y=$'\033[33m'; R=$'\033[31m'; G=$'\033[32m'; N=$'\033[0m'
else
	B=""; Y=""; R=""; G=""; N=""
fi

log() { printf '%s%s%s\n' "$B" "$*" "$N"; }
warn() { printf '%s%s%s\n' "$Y" "$*" "$N"; }
err() { printf '%s%s%s\n' "$R" "$*" "$N" >&2; }
ok() { printf '%s%s%s\n' "$G" "$*" "$N"; }

FINAL="script encerrado"
on_exit() {
	local code=$?
	printf '\n%s (codigo %d)\n' "$FINAL" "$code"
	if [ "$PAUSE" = 1 ]; then
		read -r -p "Pressione Enter para fechar..." _ || true
	fi
	exit "$code"
}
trap on_exit EXIT
trap 'err "erro na linha $LINENO"' ERR

# ---------------------------------------------------------------------------
# Detecta ambiente
# ---------------------------------------------------------------------------
detect_libobs() {
	HDR=""
	for d in /usr/include/obs /usr/local/include/obs /usr/include; do
		[ -f "$d/obs-module.h" ] && {
			HDR="$d"
			break
		}
	done

	LIB=""
	for d in /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib /lib/x86_64-linux-gnu; do
		for n in libobs.so libobs.so.0; do
			[ -e "$d/$n" ] && {
				LIB="$d/$n"
				break 2
			}
		done
	done
}

diagnose() {
	log "=== Diagnostico do TopScreen ==="
	echo "uname:        $(uname -srm)"
	echo "cmake:        $(command -v cmake || echo AUSENTE)"
	echo "g++:          $(command -v g++ || echo AUSENTE)"
	echo "pkg-config:   $(command -v pkg-config || echo AUSENTE)"
	echo "qt6 (pkg):    $(pkg-config --modversion Qt6Widgets 2>/dev/null || echo AUSENTE)"
	echo "obs binario:  $(command -v obs || echo AUSENTE)"
	echo "obs versao:   $(obs --version 2>/dev/null || echo '?')"

	detect_libobs
	echo "libobs header: ${HDR:-AUSENTE}"
	echo "libobs lib:    ${LIB:-AUSENTE}"

	echo "plugin dirs:"
	for d in /usr/lib/*/obs-plugins /usr/lib/obs-plugins /usr/local/lib/obs-plugins; do
		[ -d "$d" ] && echo "  $d ($(ls -1 "$d" 2>/dev/null | wc -l) plugins)"
	done
	echo "log dir:      ${HOME}/.config/obs-studio/logs"
	echo
	if [ -n "$HDR" ] && [ -n "$LIB" ]; then
		ok "libobs OK: o build deve funcionar."
	else
		warn "libobs (dev) ausente. Instale: sudo apt install libobs-dev"
		warn "ou use o PPA: sudo add-apt-repository ppa:obsproject/obs-studio && sudo apt update && sudo apt install libobs-dev"
	fi
	FINAL="diagnostico concluido"
}

# ---------------------------------------------------------------------------
if [ "$DIAGNOSE" = 1 ]; then
	diagnose
	exit 0
fi

command -v cmake >/dev/null || {
	err "cmake nao encontrado."
	err "  Instale: sudo apt install cmake build-essential"
	FINAL="falhou: falta cmake"
	exit 1
}
command -v g++ >/dev/null || {
	err "g++ nao encontrado. Instale: sudo apt install build-essential"
	FINAL="falhou: falta g++"
	exit 1
}

detect_libobs
if [ -z "$HDR" ] || [ -z "$LIB" ]; then
	err "libobs (dev) nao encontrado."
	err "  Ubuntu/Debian: sudo apt install libobs-dev"
	err "  ou PPA: sudo add-apt-repository ppa:obsproject/obs-studio && sudo apt update && sudo apt install libobs-dev"
	warn "  Rode 'scripts/build-install.sh --diagnose' para ver o ambiente."
	FINAL="falhou: falta libobs-dev"
	exit 1
fi

log "libobs: headers=$HDR  lib=$LIB"

# ---------------------------------------------------------------------------
log "Configurando ..."
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
	-DLIBOBS_INCLUDE_DIR="$HDR" -DLIBOBS_LIBRARY="$LIB"
RC=$?
if [ $RC -ne 0 ]; then
	err "cmake falhou ao configurar (codigo $RC). Veja as mensagens acima."
	FINAL="falhou: configure"
	exit $RC
fi

log "Compilando (-j$JOBS) ..."
cmake --build "$BUILD" -j"$JOBS"
RC=$?
if [ $RC -ne 0 ]; then
	err "compilacao falhou (codigo $RC)."
	FINAL="falhou: compilacao"
	exit $RC
fi

SO="$BUILD/topscreen.so"
[ -f "$SO" ] || {
	err "topscreen.so nao gerado em $BUILD"
	FINAL="falhou: binario ausente"
	exit 1
}
ok "OK: $SO"

# ---------------------------------------------------------------------------
case "$MODE" in
build)
	log "Build concluido. Para instalar:"
	echo "  scripts/build-install.sh --user             # so para voce (sem sudo)"
	echo "  scripts/build-install.sh --system           # OBS do sistema (sudo)"
	echo "  scripts/build-install.sh --dir <plugins>    # portable/custom"
	FINAL="build concluido"
	;;
system)
	# Descobre a pasta de plugins que o OBS usa (multiarch no Debian/Ubuntu).
	PLUGDIR=""
	for d in /usr/lib/*/obs-plugins; do
		[ -d "$d" ] && {
			PLUGDIR="$d"
			break
		}
	done
	[ -n "$PLUGDIR" ] || PLUGDIR="/usr/lib/obs-plugins"
	DATADIR="/usr/share/obs/obs-plugins/topscreen"

	log "Instalando para o OBS do sistema:"
	echo "  plugin: $PLUGDIR/topscreen.so"
	echo "  dados:  $DATADIR/"

	sudo mkdir -p "$PLUGDIR" "$DATADIR"
	sudo cp -f "$SO" "$PLUGDIR/topscreen.so"
	sudo cp -rf "$ROOT/data/." "$DATADIR/"

	# Remove instalacao antiga no lugar errado (ex.: /usr/lib/obs-plugins).
	if [ "$PLUGDIR" != "/usr/lib/obs-plugins" ] && [ -f /usr/lib/obs-plugins/topscreen.so ]; then
		warn "removendo instalacao antiga em /usr/lib/obs-plugins/topscreen.so"
		sudo rm -f /usr/lib/obs-plugins/topscreen.so
	fi

	ok "Instalado. Reinicie o OBS."
	FINAL="instalado em $PLUGDIR"
	;;
user)
	P="$HOME/.config/obs-studio/plugins/topscreen"
	log "Instalando como plugin de usuario (sem sudo): $P"
	mkdir -p "$P/bin/64bit" "$P/data"
	cp -f "$SO" "$P/bin/64bit/topscreen.so"
	cp -rf "$ROOT/data/." "$P/data/"
	ok "Instalado. Reinicie o OBS."
	FINAL="instalado em $P"
	;;
prefix)
	log "Instalando em prefixo $TARGET_PREFIX ..."
	SUDO=""
	[ -w "$TARGET_PREFIX" ] || SUDO="sudo"
	${SUDO} cmake --install "$BUILD" --prefix "$TARGET_PREFIX"
	ok "Instalado. Reinicie o OBS."
	FINAL="instalado em $TARGET_PREFIX"
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
		ok "Dados em $DATA_DIR"
	else
		mkdir -p "$TARGET_DIR/topscreen"
		cp -rf "$ROOT/data/." "$TARGET_DIR/topscreen/"
		ok "Dados em $TARGET_DIR/topscreen"
	fi
	ok "Instalado. Reinicie o OBS."
	FINAL="instalado em $TARGET_DIR"
	;;
esac

cat <<'EOF'

Para ver o log do TopScreen (o OBS escreve no stderr):
  obs --verbose 2>&1 | grep -iE "topscreen|Loaded Modules"
  # ou abra ~/.config/obs-studio/logs/ (arquivo mais recente)
EOF
