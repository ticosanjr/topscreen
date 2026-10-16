#!/usr/bin/env bash
#
# Instala o TopScreen no Linux (jeito simples).
#
# Uso:
#   scripts/install-linux.sh            # plugin de usuario (sem sudo)
#   scripts/install-linux.sh --system   # OBS do sistema (usa sudo)
#
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODE="--user"

while [ $# -gt 0 ]; do
	case "$1" in
	--system | --user) MODE="$1"; shift ;;
	-h | --help) sed -n '2,9p' "$0"; exit 0 ;;
	*) echo "opcao desconhecida: $1"; exit 2 ;;
	esac
done

# ---- dependencias ----
missing=()
command -v cmake >/dev/null || missing+=("cmake")
command -v g++ >/dev/null || missing+=("build-essential")
[ -f /usr/include/obs/obs-module.h ] || missing+=("libobs-dev")

if [ "${#missing[@]}" -gt 0 ]; then
	echo "Faltam dependencias: ${missing[*]}"
	echo
	echo "Instale com:"
	echo "  sudo apt update && sudo apt install -y cmake build-essential libobs-dev"
	echo "  # para o dock (opcional): sudo apt install -y qt6-base-dev"
	exit 1
fi

# ---- build + install ----
"$ROOT/scripts/build-install.sh" "$MODE"
rc=$?

if [ $rc -eq 0 ]; then
	cat <<EOF

Proximo passo (teclas no Linux): aplique a regra udev para o overlay funcionar
  sudo cp "$ROOT/data/udev/99-topscreen.rules" /etc/udev/rules.d/
  sudo udevadm control --reload-rules && sudo udevadm trigger
  # relogar a sessao ajuda

No OBS: Captura de Tela -> Filtros -> + -> "TopScreen FX".
EOF
fi

exit $rc
