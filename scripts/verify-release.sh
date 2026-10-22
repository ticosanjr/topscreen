#!/usr/bin/env bash
#
# Verifies the release archives in releases/ before publishing them.
#
#   scripts/verify-release.sh              # checks the version from CMakeLists.txt
#   scripts/verify-release.sh 0.3.0        # checks an explicit version
#   scripts/verify-release.sh --dir DIR    # reads archives from DIR
#
# It checks the archive structure, the binary version string, architecture,
# required data files/locale keys, Windows launchers and (when present) that
# the packaged binaries are byte-identical to build/topscreen.so/.dll.
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/releases"
VER=""

while [ $# -gt 0 ]; do
	case "$1" in
	--dir) OUT="$2"; shift 2 ;;
	-h | --help) sed -n '2,12p' "$0"; exit 0 ;;
	-*) echo "unknown option: $1" >&2; exit 2 ;;
	*) VER="$1"; shift ;;
	esac
done

if [ -z "$VER" ]; then
	VER=$(grep -E '^project\(TopScreen VERSION' "$ROOT/CMakeLists.txt" |
		grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
fi
[ -n "$VER" ] || { echo "could not determine the version" >&2; exit 1; }

LINUX="$OUT/topscreen-$VER-linux.tar.gz"
WINDOWS="$OUT/topscreen-$VER-windows.zip"
fails=0
ok() { printf '  OK   %s\n' "$*"; }
fail() { printf '  FAIL %s\n' "$*" >&2; fails=$((fails + 1)); }

for f in "$LINUX" "$WINDOWS"; do
	[ -s "$f" ] && ok "archive exists: $(basename "$f")" || fail "missing archive: $f"
done
[ "$fails" -eq 0 ] || exit 1

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

mkdir -p "$STAGE/linux" "$STAGE/windows"
tar -xzf "$LINUX" -C "$STAGE/linux"
unzip -q "$WINDOWS" -d "$STAGE/windows"

LIN="$STAGE/linux/topscreen"
WIN="$STAGE/windows/topscreen"

check_binary() {
	local path="$1" kind="$2"
	if [ ! -s "$path" ]; then
		fail "missing binary: $path"
		return
	fi
	if grep -aq "loaded v$VER" "$path"; then
		ok "$kind binary contains loaded v$VER"
	else
		fail "$kind binary does not contain loaded v$VER"
	fi
	local desc
	desc=$(file -b "$path")
	case "$kind" in
	Linux)
		[[ "$desc" == *"ELF 64-bit"* ]] && ok "Linux binary is ELF 64-bit" ||
			fail "unexpected Linux binary format: $desc"
		;;
	Windows)
		[[ "$desc" == *"PE32+"* ]] && ok "Windows binary is PE32+" ||
			fail "unexpected Windows binary format: $desc"
		;;
	esac
}

check_binary "$LIN/bin/64bit/topscreen.so" Linux
check_binary "$WIN/bin/64bit/topscreen.dll" Windows

for tree in "$LIN" "$WIN"; do
	for rel in effects/topscreen.effect effects/text.effect locale/en-US.ini \
		sounds/click-soft.wav sounds/click-mouse-right.wav; do
		[ -s "$tree/data/$rel" ] && ok "data: $rel" ||
			fail "missing/empty data file: $tree/data/$rel"
	done
	if grep -q '^Highlight.HotkeyEnabled=' "$tree/data/locale/en-US.ini" &&
		grep -q '^Hotkey.ToggleHighlight=' "$tree/data/locale/en-US.ini"; then
		ok "locale contains the hotkey highlight keys"
	else
		fail "locale is missing Highlight.HotkeyEnabled/Hotkey.ToggleHighlight"
	fi
done

for f in install.bat uninstall.bat; do
	[ -s "$WIN/$f" ] && ok "Windows launcher: $f" || fail "missing Windows launcher: $f"
done

if [ -f "$ROOT/build/topscreen.so" ]; then
	cmp -s "$ROOT/build/topscreen.so" "$LIN/bin/64bit/topscreen.so" &&
		ok "Linux package matches build/topscreen.so" ||
		fail "Linux package differs from build/topscreen.so"
fi
if [ -f "$ROOT/build/topscreen.dll" ]; then
	cmp -s "$ROOT/build/topscreen.dll" "$WIN/bin/64bit/topscreen.dll" &&
		ok "Windows package matches build/topscreen.dll" ||
		fail "Windows package differs from build/topscreen.dll"
fi

echo
sha256sum "$LINUX" "$WINDOWS"
echo
if [ "$fails" -ne 0 ]; then
	echo "release verification failed ($fails problem(s))" >&2
	exit 1
fi
echo "release verification passed for v$VER"
