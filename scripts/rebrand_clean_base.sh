#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

# Rename the clean upstream SD directory before changing textual references.
if [[ -d config/status-monitor-deux ]]; then
    mv config/status-monitor-deux config/status-monitor
fi

# Restrict replacement to known text source files and never touch Git metadata.
while IFS= read -r -d '' file; do
    sed -i \
        -e 's/status-monitor-deux/status-monitor/g' \
        -e 's/Status Monitor Deux/Ryazha Status Monitor/g' \
        "$file"
done < <(find . -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.ini' -o -name '*.md' -o -name 'Makefile' \) -not -path './.git/*' -print0)

# Preserve the existing test-release version until the user requests a new tag.
sed -i 's/^APP_VERSION[[:space:]]*:=.*/APP_VERSION\t:=\t1.5.0/' Makefile

printf 'Rebrand complete.\n'
