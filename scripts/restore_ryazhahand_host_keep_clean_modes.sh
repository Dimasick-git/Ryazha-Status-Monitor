#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
backup=/home/ubuntu/backups/ryazha-status-monitor-pre-rebuild/Ryazha-Status-Monitor.git
commit=1cd9a4a96df48cf4ff0efad8c5c697ef1bd1f2c6
library_commit=890f5f294e9ae4109e5df0ae76321b3d3ba83b6d
scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT

# Keep the new clean mode pack and the new Russian fallback locale untouched.
cp "$root/include/defaultLocale.ini" "$scratch/defaultLocale.ini"
cp -a "$root/config/status-monitor" "$scratch/status-monitor"
cp -a "$root/modes" "$scratch/modes"

# Restore only the already-proven host, parser, UI and build integration.
git --git-dir="$backup" archive "$commit" \
    Makefile CMakeLists.txt .gitmodules include source tests \
    | tar -x -C "$root"

# Restore the clean pack and Russian first-run configuration after host extraction.
rm -rf "$root/config/status-monitor" "$root/modes"
mkdir -p "$root/config"
cp -a "$scratch/status-monitor" "$root/config/status-monitor"
cp -a "$scratch/modes" "$root/modes"
cp "$scratch/defaultLocale.ini" "$root/include/defaultLocale.ini"

# Replace clean libtesla with the owner-maintained libryazhahand submodule.
rm -rf "$root/lib/libtesla" "$root/lib/libryazhahand" "$root/.git/modules/lib/libryazhahand"
git -C "$root" update-index --force-remove lib/libtesla 2>/dev/null || true
git -C "$root" update-index --force-remove lib/libryazhahand 2>/dev/null || true
git clone https://github.com/Dimasick-git/libryazhahand.git "$root/lib/libryazhahand"
git -C "$root/lib/libryazhahand" checkout "$library_commit"
git -C "$root" add .gitmodules
git -C "$root" add -f lib/libryazhahand

echo "Restored libryazhahand-compatible host while keeping clean official modes."
