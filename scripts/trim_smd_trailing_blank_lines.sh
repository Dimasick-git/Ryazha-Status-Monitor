#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
find "$root/modes" -type f -name '*.smd' -print0 | while IFS= read -r -d '' file; do
    tmp="${file}.tmp"
    awk '
        { lines[NR] = $0; if ($0 != "") last = NR }
        END { for (i = 1; i <= last; ++i) print lines[i] }
    ' "$file" > "$tmp"
    mv "$tmp" "$file"
done
