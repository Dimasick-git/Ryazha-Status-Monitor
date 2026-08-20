#!/usr/bin/env python3
"""Lightweight, deterministic checks for the Ryazha Status Monitor source tree."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"validation error: {message}")


def main() -> None:
    russian_locale = ROOT / "lang" / "ru.json"
    with russian_locale.open("r", encoding="utf-8") as locale_file:
        translations = json.load(locale_file)

    require(isinstance(translations, dict) and translations, "Russian locale must be a non-empty JSON object")
    for key in ("Full", "Mini", "Micro", "FPS Counter", "FPS Graph", "Configuration"):
        require(key in translations, f"missing Russian translation key: {key}")
    expected_terms = {
        "CPU": "ЦПУ",
        "GPU": "ГПУ",
        "RAM": "ОЗУ",
        "Checking for game...": "Проверка игры...",
    }
    for key, value in expected_terms.items():
        require(translations.get(key) == value, f"incorrect Russian translation for {key}")

    makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
    require("lib/libryazhahand/ryazhahand.mk" in makefile, "Makefile must include libryazhahand")
    require("lib/libultrahand/ultrahand.mk" not in makefile, "Makefile must not include libultrahand")

    required_modes = {
        "Full.hpp",
        "Mini.hpp",
        "Micro.hpp",
        "FPS_Counter.hpp",
        "FPS_Graph.hpp",
        "Resolutions.hpp",
        "Misc.hpp",
        "Battery.hpp",
    }
    present_modes = {path.name for path in (ROOT / "source" / "modes").glob("*.hpp")}
    missing_modes = sorted(required_modes - present_modes)
    require(not missing_modes, f"missing ppkantorski mode sources: {', '.join(missing_modes)}")

    for name in ("Mini.hpp", "FPS_Counter.hpp", "FPS_Graph.hpp", "Resolutions.hpp"):
        content = (ROOT / "source" / "modes" / name).read_text(encoding="utf-8")
        require("hidGetTouchScreenStates(&rawTouchState, 1)" in content,
                f"{name} must read touchscreen state directly")
    for name in ("Mini.hpp", "FPS_Counter.hpp", "FPS_Graph.hpp", "Resolutions.hpp"):
        content = (ROOT / "source" / "modes" / name).read_text(encoding="utf-8")
        require("rawTouchState.count >= 2" in content,
                f"{name} must support two-finger pinch scaling")

    mini = (ROOT / "source" / "modes" / "Mini.hpp").read_text(encoding="utf-8")
    require("Initialized = false;" in mini,
            "Mini must invalidate its geometry cache after pinch scaling")
    full = (ROOT / "source" / "modes" / "Full.hpp").read_text(encoding="utf-8")
    for title in ("ЦПУ", "ГПУ", "ОЗУ", "Игра", "Плата и питание"):
        require(f'drawCard(' in full and f'"{title}"' in full,
                f"Full card layout is missing {title}")
    require("makeSwitch2Wheel(" in full,
            "Full card layout must use the Ryazha-clk dynamic border palette")

    print("Ryazha Status Monitor source validation passed.")


if __name__ == "__main__":
    main()
