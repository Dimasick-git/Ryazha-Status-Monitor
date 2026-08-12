#!/usr/bin/env bash
# Run host-side SMD parser tests and validate the packaged overlay, if present.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build-tests}"
cd "$ROOT_DIR"

command -v cmake >/dev/null || {
    printf 'CMake is required to run parser tests.\n' >&2
    exit 1
}

cmake -S . -B "$BUILD_DIR" -DSMD_BUILD_TESTS=ON -DSMD_WARNINGS_AS_ERRORS=ON
cmake --build "$BUILD_DIR" --parallel "$(nproc)"
ctest --test-dir "$BUILD_DIR" --output-on-failure

if [[ -f "Ryazha-Status-Monitor.zip" ]]; then
    zipinfo -1 "Ryazha-Status-Monitor.zip" | grep -qx 'switch/.overlays/Ryazha-Status-Monitor.ovl'
    zipinfo -1 "Ryazha-Status-Monitor.zip" | grep -qx 'config/status-monitor/locale.ini'
    printf 'Distribution archive layout: valid\n'
fi
