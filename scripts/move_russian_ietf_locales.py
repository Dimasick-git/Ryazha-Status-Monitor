#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

for path in sorted((ROOT / "modes").rglob("*.smd")):
    lines = path.read_text(encoding="utf-8").splitlines()
    russian = [line for line in lines if line.startswith("IETF_LOCALE{") and '"RU-RU"' in line]
    if not russian:
        continue

    lines = [line for line in lines if line not in russian]
    try:
        start_index = next(i for i, line in enumerate(lines) if line.strip() == "Start:")
    except StopIteration:
        raise SystemExit(f"Missing Start: in {path}")

    lines[start_index:start_index] = russian + [""]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")

print("Russian IETF locale directives moved before SMD commands.")
