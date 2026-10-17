#!/usr/bin/env bash
#
# Installs TopScreen on Linux (simple way).
#
# Usage:
#   scripts/install-linux.sh            # user plugin (no sudo)
#   scripts/install-linux.sh --system   # system OBS (uses sudo)
#
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODE="--user"

while [ $# -gt 0 ]; do
	case "$1" in
	--system | --user) MODE="$1"; shift ;;
	-h | --help) sed -n '2,9p' "$0"; exit 0 ;;
	*) echo "unknown option: $1"; exit 2 ;;
	esac
done

# ---- dependencies ----
missing=()
command -v cmake >/dev/null || missing+=("cmake")
command -v g++ >/dev/null || missing+=("build-essential")
[ -f /usr/include/obs/obs-module.h ] || missing+=("libobs-dev")

if [ "${#missing[@]}" -gt 0 ]; then
	echo "Missing dependencies: ${missing[*]}"
	echo
	echo "Install with:"
	echo "  sudo apt update && sudo apt install -y cmake build-essential libobs-dev"
	echo "  # for the dock (optional): sudo apt install -y qt6-base-dev"
	exit 1
fi

# ---- build + install ----
"$ROOT/scripts/build-install.sh" "$MODE"
rc=$?

if [ $rc -eq 0 ]; then
	cat <<EOF

Next step (keys on Linux): apply the udev rule so the overlay works
  sudo cp "$ROOT/data/udev/99-topscreen.rules" /etc/udev/rules.d/
  sudo udevadm control --reload-rules && sudo udevadm trigger
  # logging the session back in helps

In OBS: Screen Capture -> Filters -> + -> "TopScreen FX".
EOF
fi

exit $rc
