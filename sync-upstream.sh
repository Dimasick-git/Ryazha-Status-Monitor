#!/usr/bin/env bash
# sync-upstream.sh — Sync changes from masagrator/Status-Monitor-Overlay
# Applies upstream changes while protecting our customizations.
#
# Usage:
#   ./sync-upstream.sh           — sync and show what changed
#   ./sync-upstream.sh --dry-run — only show what would change, don't touch files

set -euo pipefail

UPSTREAM_REMOTE="upstream"
UPSTREAM_URL="https://github.com/masagrator/Status-Monitor-Overlay.git"
UPSTREAM_BRANCH="master"
SYNC_FILE=".upstream-sync"
DRY_RUN=false

[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=true

# ─── Protected files ───────────────────────────────────────────────────────
# These are never overwritten from upstream (our custom code lives here).
PROTECTED=(
    # Library: libryazhahand replaces libultrahand/libtesla
    "lib/libryazhahand"
    ".gitmodules"
    # Build & CI
    "Makefile"
    ".github"
    # Docs
    "README.md"
    # Scripts
    "sync-upstream.sh"
    "scripts"
    ".upstream-sync"
    # Lang files (our translations)
    "lang"
    # Config template
    "config"
    # Docker setup
    "Dockerfile"
    "docker-compose.yml"
)

# ─── Patches always re-applied after merge ─────────────────────────────────
apply_permanent_patches() {
    # Ensure libryazhahand is used, not libultrahand
    if [[ -f Makefile ]]; then
        sed -i 's|include.*libultrahand.*\.mk|include $(TOPDIR)/lib/libryazhahand/ryazhahand.mk|g' Makefile
        sed -i 's|${TOPDIR}|$(TOPDIR)|g' Makefile
        sed -i 's|Ultrahand signature has been added|Ryazhahand signature has been added|g' Makefile
    fi
}

# ─── Helpers ───────────────────────────────────────────────────────────────
is_protected() {
    local f="$1"
    for p in "${PROTECTED[@]}"; do
        [[ "$f" == "$p" || "$f" == "$p/"* ]] && return 0
    done
    return 1
}

green()  { echo -e "\033[32m$*\033[0m"; }
yellow() { echo -e "\033[33m$*\033[0m"; }
red()    { echo -e "\033[31m$*\033[0m"; }
bold()   { echo -e "\033[1m$*\033[0m"; }

# ─── Setup remote ──────────────────────────────────────────────────────────
if ! git remote get-url "$UPSTREAM_REMOTE" &>/dev/null; then
    echo "→ Adding upstream remote: $UPSTREAM_URL"
    git remote add "$UPSTREAM_REMOTE" "$UPSTREAM_URL"
fi

echo "→ Fetching upstream..."
git fetch "$UPSTREAM_REMOTE" --no-tags -q

UPSTREAM_HEAD=$(git rev-parse "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH")

# ─── Determine base commit ─────────────────────────────────────────────────
if [[ -f "$SYNC_FILE" ]]; then
    BASE=$(cat "$SYNC_FILE" | tr -d '[:space:]')
    if ! git cat-file -e "$BASE^{commit}" 2>/dev/null; then
        BASE=$(git merge-base HEAD "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH" 2>/dev/null \
            || git rev-list --max-parents=0 "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH")
    fi
    bold "Last synced upstream: ${BASE:0:12}"
else
    BASE=$(git merge-base HEAD "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH" 2>/dev/null \
        || git rev-list --max-parents=0 "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH")
    bold "First run — base: ${BASE:0:12}"
fi

if [[ "$BASE" == "$UPSTREAM_HEAD" ]]; then
    green "✓ Already up to date with upstream."
    exit 0
fi

bold "\nNew upstream commits:"
git log --oneline "$BASE..$UPSTREAM_HEAD"

CHANGED=$(git diff --name-only --diff-filter=AM "$BASE" "$UPSTREAM_HEAD" || true)

if [[ -z "$CHANGED" ]]; then
    green "\n✓ No file changes to apply."
    echo "$UPSTREAM_HEAD" > "$SYNC_FILE"
    exit 0
fi

echo ""
bold "Applying changes..."
echo ""

APPLIED=0; SKIPPED=0; CONFLICTS=()

while IFS= read -r file; do
    [[ -z "$file" ]] && continue

    if is_protected "$file"; then
        yellow "  [skip protected]   $file"
        ((SKIPPED++)) || true
        continue
    fi

    if ! git show "$UPSTREAM_HEAD:$file" &>/dev/null; then
        yellow "  [skip missing]     $file"
        continue
    fi

    if $DRY_RUN; then
        echo "  [would apply]      $file"
        continue
    fi

    if [[ ! -f "$file" ]]; then
        mkdir -p "$(dirname "$file")"
        git show "$UPSTREAM_HEAD:$file" > "$file"
        green "  [add]              $file"
        ((APPLIED++)) || true
        continue
    fi

    TMP=$(mktemp -d)
    git show "$BASE:$file" > "$TMP/base" 2>/dev/null || touch "$TMP/base"
    cp "$file" "$TMP/ours"
    git show "$UPSTREAM_HEAD:$file" > "$TMP/theirs"

    if git merge-file -q "$TMP/ours" "$TMP/base" "$TMP/theirs" 2>/dev/null; then
        cp "$TMP/ours" "$file"
        green "  [merge ok]         $file"
        ((APPLIED++)) || true
    else
        cp "$TMP/ours" "$file"
        red "  [conflict]         $file"
        CONFLICTS+=("$file")
    fi
    rm -rf "$TMP"

done <<< "$CHANGED"

if ! $DRY_RUN; then
    apply_permanent_patches
fi

echo ""
echo "────────────────────────────────────────────"
bold "Applied: $APPLIED  |  Skipped: $SKIPPED  |  Conflicts: ${#CONFLICTS[@]}"

if [[ ${#CONFLICTS[@]} -gt 0 ]]; then
    echo ""
    red "Files with conflicts (resolve <<<<<<< markers, then commit):"
    for f in "${CONFLICTS[@]}"; do red "  $f"; done
    echo ""
    echo "After resolving, run:"
    echo "  echo $UPSTREAM_HEAD > $SYNC_FILE"
    echo "  git add . && git commit -m 'chore: sync upstream'"
    exit 1
fi

if ! $DRY_RUN; then
    echo "$UPSTREAM_HEAD" > "$SYNC_FILE"
    green "✓ Sync point saved: ${UPSTREAM_HEAD:0:12}"
    echo ""
    echo "Review changes, then commit:"
    echo "  git diff"
    echo "  git add -p && git commit -m \"chore: sync upstream $(date +%Y-%m-%d)\""
fi
