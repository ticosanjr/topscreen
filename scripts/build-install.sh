#!/usr/bin/env bash
#
# Builds (and optionally installs) TopScreen on Linux, detecting libobs.
#
# Usage:
#   scripts/build-install.sh                 # build only
#   scripts/build-install.sh --diagnose      # show the environment (no build)
#   scripts/build-install.sh --system        # install into the system OBS (sudo)
#   scripts/build-install.sh --prefix DIR    # install under DIR (e.g. OBS build)
#   scripts/build-install.sh --dir DIR       # copy to the plugins folder DIR
#   scripts/build-install.sh --pause         # wait for Enter at the end (double-click)
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
	*) echo "unknown option: $1"; exit 2 ;;
	esac
done

# Style off if not a terminal.
if [ -t 1 ]; then
	B=$'\033[1m'; Y=$'\033[33m'; R=$'\033[31m'; G=$'\033[32m'; N=$'\033[0m'
else
	B=""; Y=""; R=""; G=""; N=""
fi

log() { printf '%s%s%s\n' "$B" "$*" "$N"; }
warn() { printf '%s%s%s\n' "$Y" "$*" "$N"; }
err() { printf '%s%s%s\n' "$R" "$*" "$N" >&2; }
ok() { printf '%s%s%s\n' "$G" "$*" "$N"; }

FINAL="script finished"
on_exit() {
	local code=$?
	printf '\n%s (code %d)\n' "$FINAL" "$code"
	if [ "$PAUSE" = 1 ]; then
		read -r -p "Press Enter to close..." _ || true
	fi
	exit "$code"
}
trap on_exit EXIT
trap 'err "error on line $LINENO"' ERR

# ---------------------------------------------------------------------------
# Detects environment
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
	log "=== TopScreen diagnostics ==="
	echo "uname:        $(uname -srm)"
	echo "cmake:        $(command -v cmake || echo MISSING)"
	echo "g++:          $(command -v g++ || echo MISSING)"
	echo "pkg-config:   $(command -v pkg-config || echo MISSING)"
	echo "qt6 (pkg):    $(pkg-config --modversion Qt6Widgets 2>/dev/null || echo MISSING)"
	echo "obs binary:   $(command -v obs || echo MISSING)"
	echo "obs version:  $(obs --version 2>/dev/null || echo '?')"

	detect_libobs
	echo "libobs header: ${HDR:-MISSING}"
	echo "libobs lib:    ${LIB:-MISSING}"

	echo "plugin dirs:"
	for d in /usr/lib/*/obs-plugins /usr/lib/obs-plugins /usr/local/lib/obs-plugins; do
		[ -d "$d" ] && echo "  $d ($(ls -1 "$d" 2>/dev/null | wc -l) plugins)"
	done
	echo "log dir:      ${HOME}/.config/obs-studio/logs"
	echo
	if [ -n "$HDR" ] && [ -n "$LIB" ]; then
		ok "libobs OK: the build should work."
	else
		warn "libobs (dev) missing. Install: sudo apt install libobs-dev"
		warn "or use the PPA: sudo add-apt-repository ppa:obsproject/obs-studio && sudo apt update && sudo apt install libobs-dev"
	fi
	FINAL="diagnostics complete"
}

# ---------------------------------------------------------------------------
if [ "$DIAGNOSE" = 1 ]; then
	diagnose
	exit 0
fi

command -v cmake >/dev/null || {
	err "cmake not found."
	err "  Install: sudo apt install cmake build-essential"
	FINAL="failed: cmake missing"
	exit 1
}
command -v g++ >/dev/null || {
	err "g++ not found. Install: sudo apt install build-essential"
	FINAL="failed: g++ missing"
	exit 1
}

detect_libobs
if [ -z "$HDR" ] || [ -z "$LIB" ]; then
	err "libobs (dev) not found."
	err "  Ubuntu/Debian: sudo apt install libobs-dev"
	err "  or PPA: sudo add-apt-repository ppa:obsproject/obs-studio && sudo apt update && sudo apt install libobs-dev"
	warn "  Run 'scripts/build-install.sh --diagnose' to see the environment."
	FINAL="failed: libobs-dev missing"
	exit 1
fi

log "libobs: headers=$HDR  lib=$LIB"

# ---------------------------------------------------------------------------
log "Configuring ..."
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
	-DLIBOBS_INCLUDE_DIR="$HDR" -DLIBOBS_LIBRARY="$LIB"
RC=$?
if [ $RC -ne 0 ]; then
	err "cmake failed to configure (code $RC). See the messages above."
	FINAL="failed: configure"
	exit $RC
fi

log "Building (-j$JOBS) ..."
cmake --build "$BUILD" -j"$JOBS"
RC=$?
if [ $RC -ne 0 ]; then
	err "build failed (code $RC)."
	FINAL="failed: build"
	exit $RC
fi

SO="$BUILD/topscreen.so"
[ -f "$SO" ] || {
	err "topscreen.so not generated in $BUILD"
	FINAL="failed: binary missing"
	exit 1
}
ok "OK: $SO"

# ---------------------------------------------------------------------------
# Verifies an installed copy: binary + data layout + version.
verify_install() {
	# $1 = plugin binary, $2 = data dir
	local bin="$1" data="$2" fail=0
	[ -f "$bin" ] || { err "missing binary: $bin"; fail=1; }
	for f in locale/en-US.ini presets/presentation.json presets/demo.json presets/tutorial.json \
		effects/topscreen.effect effects/text.effect cursors/atlas.rgba cursors/atlas.json; do
		[ -f "$data/$f" ] || { err "missing data file: $data/$f"; fail=1; }
	done
	for d in sounds fonts wallpapers; do
		[ -d "$data/$d" ] || { err "missing data dir: $data/$d"; fail=1; }
	done
	[ "$fail" -ne 0 ] && return 1
	local ver
	ver=$(strings "$bin" 2>/dev/null | grep -oE "loaded v[0-9]+\.[0-9]+\.[0-9]+" | head -1)
	log "Verified: $bin${ver:+ ($ver)} + $(ls "$data/wallpapers"/*.jpg 2>/dev/null | wc -l) wallpapers"
	return 0
}

# ---------------------------------------------------------------------------
case "$MODE" in
build)
	log "Build complete. To install:"
	echo "  scripts/build-install.sh --user             # just for you (no sudo)"
	echo "  scripts/build-install.sh --system           # system OBS (sudo)"
	echo "  scripts/build-install.sh --dir <plugins>    # portable/custom"
	FINAL="build complete"
	;;
system)
	# Finds the plugins folder OBS uses (multiarch on Debian/Ubuntu).
	PLUGDIR=""
	for d in /usr/lib/*/obs-plugins; do
		[ -d "$d" ] && {
			PLUGDIR="$d"
			break
		}
	done
	[ -n "$PLUGDIR" ] || PLUGDIR="/usr/lib/obs-plugins"
	DATADIR="/usr/share/obs/obs-plugins/topscreen"

	log "Installing into the system OBS:"
	echo "  plugin: $PLUGDIR/topscreen.so"
	echo "  data:   $DATADIR/"

	sudo mkdir -p "$PLUGDIR"
	# Full sync: drop stale files from previous versions (e.g. old locales).
	sudo rm -rf "$DATADIR"
	sudo mkdir -p "$DATADIR"
	sudo cp -f "$SO" "$PLUGDIR/topscreen.so"
	sudo cp -rf "$ROOT/data/." "$DATADIR/"

	# Removes an old installation in the wrong place (e.g. /usr/lib/obs-plugins).
	if [ "$PLUGDIR" != "/usr/lib/obs-plugins" ] && [ -f /usr/lib/obs-plugins/topscreen.so ]; then
		warn "removing old installation at /usr/lib/obs-plugins/topscreen.so"
		sudo rm -f /usr/lib/obs-plugins/topscreen.so
	fi

	ok "Installed. Restart OBS."
	FINAL="installed in $PLUGDIR"
	verify_install "$PLUGDIR/topscreen.so" "$DATADIR" || FINAL="installed in $PLUGDIR (verify FAILED)"
	;;
user)
	P="$HOME/.config/obs-studio/plugins/topscreen"
	log "Installing as a user plugin (no sudo): $P"
	# Full sync: drop stale files from previous versions (e.g. old locales).
	rm -rf "$P/data"
	mkdir -p "$P/bin/64bit" "$P/data"
	cp -f "$SO" "$P/bin/64bit/topscreen.so"
	cp -rf "$ROOT/data/." "$P/data/"
	ok "Installed. Restart OBS."
	FINAL="installed in $P"
	verify_install "$P/bin/64bit/topscreen.so" "$P/data" || FINAL="installed in $P (verify FAILED)"
	;;
prefix)
	log "Installing into prefix $TARGET_PREFIX ..."
	SUDO=""
	[ -w "$TARGET_PREFIX" ] || SUDO="sudo"
	${SUDO} cmake --install "$BUILD" --prefix "$TARGET_PREFIX"
	ok "Installed. Restart OBS."
	FINAL="installed in $TARGET_PREFIX"
	;;
dir)
	[ -n "$TARGET_DIR" ] || {
		err "--dir empty"
		exit 2
	}
	log "Copying to $TARGET_DIR ..."
	mkdir -p "$TARGET_DIR"
	cp -f "$SO" "$TARGET_DIR/topscreen.so"
	if [ -n "$DATA_DIR" ]; then
		rm -rf "$DATA_DIR"
		mkdir -p "$DATA_DIR"
		cp -rf "$ROOT/data/." "$DATA_DIR/"
		ok "Data in $DATA_DIR"
	else
		rm -rf "$TARGET_DIR/topscreen"
		mkdir -p "$TARGET_DIR/topscreen"
		cp -rf "$ROOT/data/." "$TARGET_DIR/topscreen/"
		ok "Data in $TARGET_DIR/topscreen"
	fi
	ok "Installed. Restart OBS."
	FINAL="installed in $TARGET_DIR"
	if [ -n "$DATA_DIR" ]; then
		verify_install "$TARGET_DIR/topscreen.so" "$DATA_DIR" || FINAL="installed (verify FAILED)"
	fi
	;;
esac

cat <<'EOF'

To see the TopScreen log (OBS writes to stderr):
  obs --verbose 2>&1 | grep -iE "topscreen|Loaded Modules"
  # or open ~/.config/obs-studio/logs/ (most recent file)
EOF
