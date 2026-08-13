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

# `make` runs in the devkitA64 container where CXX points at the Switch
# cross-compiler. Parser tests are host executables, so always select a native
# compiler explicitly; otherwise CTest attempts to execute AArch64 binaries.
HOST_CXX="${SMD_HOST_CXX:-}"
HOST_CC="${SMD_HOST_CC:-}"
if [[ -z "$HOST_CXX" ]]; then
    if [[ -x /usr/bin/g++ ]]; then HOST_CXX=/usr/bin/g++; else HOST_CXX="$(command -v c++)"; fi
fi
if [[ -z "$HOST_CC" ]]; then
    if [[ -x /usr/bin/gcc ]]; then HOST_CC=/usr/bin/gcc; else HOST_CC="$(command -v cc)"; fi
fi
[[ -x "$HOST_CXX" ]] || { printf 'A native C++ compiler is required for parser tests.\n' >&2; exit 1; }
[[ -x "$HOST_CC" ]] || { printf 'A native C compiler is required for parser tests.\n' >&2; exit 1; }

cmake -S . -B "$BUILD_DIR" -DSMD_BUILD_TESTS=ON -DSMD_WARNINGS_AS_ERRORS=ON \
    -DCMAKE_CXX_COMPILER="$HOST_CXX" -DCMAKE_C_COMPILER="$HOST_CC"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"
ctest --test-dir "$BUILD_DIR" --output-on-failure

if [[ -f "Ryazha-Status-Monitor.zip" ]]; then
    zipinfo -1 "Ryazha-Status-Monitor.zip" | grep -qx 'switch/.overlays/Ryazha-Status-Monitor.ovl'
    zipinfo -1 "Ryazha-Status-Monitor.zip" | grep -qx 'config/status-monitor/locale.ini'
    printf 'Distribution archive layout: valid\n'
fi
