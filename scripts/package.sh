#!/usr/bin/env bash
#
# Packs release archives from an already built tree (no compiler needed).
#
# Usage:
#   scripts/package.sh                  # packs whatever was built into dist/
#   scripts/package.sh --build-dir DIR   # use another build dir (default: build)
#
# Output:
#   dist/topscreen-<version>-linux.tar.gz     (topscreen/{bin/64bit/*.so,data/...})
#   dist/topscreen-<version>-windows.zip      (topscreen/{bin/64bit/*.dll,data/...})
#
# Install from a package (no build):
#   Linux:  tar -xzf topscreen-<v>-linux.tar.gz -C ~/.config/obs-studio/plugins/
#   Windows: extract topscreen-<v>-windows.zip into %ProgramData%\obs-studio\plugins\
#            (portable OBS: copy bin\64bit\topscreen.dll to obs-plugins\64bit and
#            data\* to data\obs-plugins\topscreen)
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
OUT="$ROOT/dist"

while [ $# -gt 0 ]; do
	case "$1" in
	--build-dir) BUILD="$2"; shift 2 ;;
	-h | --help) sed -n '2,17p' "$0"; exit 0 ;;
	*) echo "unknown option: $1"; exit 2 ;;
	esac
done

VER=$(grep -E '^project\(TopScreen VERSION' "$ROOT/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
[ -n "$VER" ] || { echo "could not read version from CMakeLists.txt"; exit 1; }

command -v zip >/dev/null || { echo "zip not found (needed for the Windows package)"; exit 1; }

mkdir -p "$OUT"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

pack_common() {
	# $1 = stage topscreen dir (already has bin lib); copies data/
	mkdir -p "$1/data"
	cp -r "$ROOT/data/." "$1/data/"
}

made=0

if [ -f "$BUILD/topscreen.so" ]; then
	T="topscreen-$VER-linux"
	mkdir -p "$STAGE/$T/topscreen/bin/64bit"
	cp -f "$BUILD/topscreen.so" "$STAGE/$T/topscreen/bin/64bit/"
	pack_common "$STAGE/$T/topscreen"
	tar -czf "$OUT/$T.tar.gz" -C "$STAGE/$T" topscreen
	echo "OK: $OUT/$T.tar.gz"
	made=1
fi

for dll in "$BUILD/Release/topscreen.dll" "$BUILD/topscreen.dll"; do
	if [ -f "$dll" ]; then
		T="topscreen-$VER-windows"
		mkdir -p "$STAGE/$T/topscreen/bin/64bit"
		cp -f "$dll" "$STAGE/$T/topscreen/bin/64bit/"
		pack_common "$STAGE/$T/topscreen"
		(cd "$STAGE/$T" && zip -qr "$OUT/$T.zip" topscreen)
		echo "OK: $OUT/$T.zip"
		made=1
		break
	fi
done

[ "$made" -eq 1 ] || {
	echo "nothing to pack: no topscreen.so or topscreen.dll in $BUILD"
	echo "build first (Linux: scripts/build-install.sh; Windows: install-windows.ps1)"
	exit 1
}
