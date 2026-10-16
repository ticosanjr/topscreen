#!/usr/bin/env bash
#
# Remove o TopScreen do Linux.
#
# Uso:
#   scripts/uninstall-linux.sh            # remove do sistema E do usuario
#   scripts/uninstall-linux.sh --system   # so do sistema (sudo)  <- resolve "Duplicate library"
#   scripts/uninstall-linux.sh --user     # so do usuario
#
set -uo pipefail

MODE="all"
case "${1:-}" in
--system) MODE="system" ;;
--user) MODE="user" ;;
--all | "") MODE="all" ;;
-h | --help) sed -n '2,9p' "$0"; exit 0 ;;
*) echo "opcao desconhecida: $1"; exit 2 ;;
esac

if [ "$MODE" = "system" ] || [ "$MODE" = "all" ]; then
	echo "== removendo do sistema (sudo) =="
	sudo rm -f /usr/lib/*/obs-plugins/topscreen.so /usr/lib/obs-plugins/topscreen.so
	sudo rm -rf /usr/share/obs/obs-plugins/topscreen
fi

if [ "$MODE" = "user" ] || [ "$MODE" = "all" ]; then
	echo "== removendo do usuario =="
	rm -rf "$HOME/.config/obs-studio/plugins/topscreen"
fi

echo "Pronto. Reinicie o OBS."
