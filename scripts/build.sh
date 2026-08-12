#!/usr/bin/env bash
# Build the Nintendo Switch overlay and the complete installable package.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [[ -z "${DEVKITPRO:-}" || ! -f "${DEVKITPRO}/libnx/switch_rules" ]]; then
    printf 'DEVKITPRO with libnx is required. See docs/BUILDING.md.\n' >&2
    exit 1
fi

make clean
make -j"$(nproc)" dist

OVL="Ryazha-Status-Monitor.ovl"
ARCHIVE="Ryazha-Status-Monitor.zip"
INSTALL_OVL="out/switch/.overlays/${OVL}"

for artifact in "$OVL" "$ARCHIVE" "$INSTALL_OVL" "out/config/status-monitor/locale.ini"; do
    if [[ ! -s "$artifact" ]]; then
        printf 'Build verification failed: missing or empty %s\n' "$artifact" >&2
        exit 1
    fi
done

printf 'Build completed: %s\n' "$OVL"
printf 'Distribution package: %s\n' "$ARCHIVE"
