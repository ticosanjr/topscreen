#!/usr/bin/env bash
#
# Keeps only the newest N release archives in releases/ (default: 3).
#
#   scripts/prune-releases.sh                 # keep the latest 3 versions
#   scripts/prune-releases.sh --keep 5
#   scripts/prune-releases.sh --dry-run
#   scripts/prune-releases.sh --dir DIR
#
# Both the Linux and Windows archive of a version are removed together. Older
# Git tags are kept; only local/downloadable archives are pruned. This is called
# automatically at the end of scripts/package.sh.
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIR="$ROOT/releases"
KEEP=3
DRY=0

while [ $# -gt 0 ]; do
	case "$1" in
	--keep) KEEP="${2:?missing value for --keep}"; shift 2 ;;
	--dir) DIR="${2:?missing value for --dir}"; shift 2 ;;
	--dry-run) DRY=1; shift ;;
	-h | --help) sed -n '2,16p' "$0"; exit 0 ;;
	*) echo "unknown option: $1" >&2; exit 2 ;;
	esac
done

case "$KEEP" in
'' | *[!0-9]*) echo "--keep must be a positive integer" >&2; exit 2 ;;
esac
[ "$KEEP" -gt 0 ] || { echo "--keep must be greater than zero" >&2; exit 2; }
[ -d "$DIR" ] || exit 0

versions="$(
	find "$DIR" -maxdepth 1 -type f \
		\( -name 'topscreen-*-linux.tar.gz' -o -name 'topscreen-*-windows.zip' \) \
		-printf '%f\n' |
		sed -n -e 's/^topscreen-\(.*\)-linux\.tar\.gz$/\1/p' \
		       -e 's/^topscreen-\(.*\)-windows\.zip$/\1/p' |
		sort -Vu
)"

total=$(printf '%s\n' "$versions" | sed '/^$/d' | wc -l)
if [ "$total" -le "$KEEP" ]; then
	echo "releases: $total version(s), keeping all (limit $KEEP)"
	exit 0
fi

remove_count=$((total - KEEP))
to_remove=$(printf '%s\n' "$versions" | sed '/^$/d' | head -n "$remove_count")

echo "releases: keeping the newest $KEEP version(s); removing $remove_count"
printf '%s\n' "$to_remove" | while IFS= read -r version; do
	[ -n "$version" ] || continue
	for file in \
		"$DIR/topscreen-$version-linux.tar.gz" \
		"$DIR/topscreen-$version-windows.zip"; do
		[ -f "$file" ] || continue
		if [ "$DRY" -eq 1 ]; then
			echo "  would remove $(basename "$file")"
		else
			rm -f -- "$file"
			echo "  removed $(basename "$file")"
		fi
	done
done
