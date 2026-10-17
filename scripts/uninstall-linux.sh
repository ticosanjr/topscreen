#!/usr/bin/env bash
#
# Removes TopScreen from Linux.
#
# Usage:
#   scripts/uninstall-linux.sh            # remove from system AND user
#   scripts/uninstall-linux.sh --system   # system only (sudo)  <- fixes "Duplicate library"
#   scripts/uninstall-linux.sh --user     # user only
#
set -uo pipefail

MODE="all"
case "${1:-}" in
--system) MODE="system" ;;
--user) MODE="user" ;;
--all | "") MODE="all" ;;
-h | --help) sed -n '2,9p' "$0"; exit 0 ;;
*) echo "unknown option: $1"; exit 2 ;;
esac

if [ "$MODE" = "system" ] || [ "$MODE" = "all" ]; then
	echo "== removing from the system (sudo) =="
	sudo rm -f /usr/lib/*/obs-plugins/topscreen.so /usr/lib/obs-plugins/topscreen.so
	sudo rm -rf /usr/share/obs/obs-plugins/topscreen
fi

if [ "$MODE" = "user" ] || [ "$MODE" = "all" ]; then
	echo "== removing from the user =="
	rm -rf "$HOME/.config/obs-studio/plugins/topscreen"
fi

echo "Done. Restart OBS."
