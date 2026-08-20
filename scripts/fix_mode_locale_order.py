#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
names = {
    "01.Full.smd": "Полный",
    "02.Mini.smd": "Мини",
    "03.Micro.smd": "Микро",
    "FPS/01.FPSGraph.smd": "График FPS",
    "FPS/02.FPSCounter.smd": "Счётчик FPS",
    "Other/01.BatteryCharger.smd": "Батарея и зарядка",
    "Other/02.Miscellaneous.smd": "Разное",
    "Other/03.GameResolutions.smd": "Разрешения игры",
}

for relative, title in names.items():
    path = ROOT / "modes" / relative
    lines = path.read_text(encoding="utf-8").splitlines()
    directive = f'NAME_LOCALE{{"RU-RU", "{title}"}}'
    lines = [line for line in lines if line != directive]

    insert_at = 0
    for i, line in enumerate(lines):
        if line.startswith("Name =") or line.startswith("NAME_LOCALE{"):
            insert_at = i + 1
        elif insert_at and line.strip() == "":
            break
    lines.insert(insert_at, directive)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")

print("Russian mode-name directives moved before SMD commands.")
