#!/usr/bin/env python3
"""Deterministic release checks for the Ryazha Status Monitor source tree."""
from __future__ import annotations

import configparser
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"validation error: {message}")


def read_text(path: Path) -> str:
    require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def validate_metadata() -> None:
    makefile = read_text(ROOT / "Makefile")
    match = re.search(r"^APP_VERSION\s*:=\s*([^\s]+)", makefile, re.MULTILINE)
    require(match is not None, "Makefile must define APP_VERSION")
    require(match.group(1) == "1.5.5", "APP_VERSION must be 1.5.5")
    require("lib/libryazhahand/ryazhahand.mk" in makefile, "Makefile must include libryazhahand")
    require("lib/libultrahand/ultrahand.mk" not in makefile, "Makefile must not include libultrahand")


def validate_locales() -> None:
    locale_paths = sorted((ROOT / "lang").glob("*.json"))
    require(len(locale_paths) == 14, f"expected 14 locale files, found {len(locale_paths)}")

    locales: dict[str, dict[str, object]] = {}
    for path in locale_paths:
        try:
            data = json.loads(read_text(path))
        except json.JSONDecodeError as error:
            raise SystemExit(f"validation error: invalid JSON in {path.name}: {error}") from error
        require(isinstance(data, dict) and data, f"{path.name} must contain a non-empty object")
        locales[path.name] = data

    english_keys = set(locales["en.json"])
    required_keys = {
        "Full",
        "Mini",
        "Micro",
        "FPS Counter",
        "FPS Graph",
        "Configuration",
        "Hide panel",
        "X Pin · Drag Move · Pinch Resize",
    }
    require(required_keys <= english_keys, "English locale is missing current interface strings")

    for name, translations in locales.items():
        keys = set(translations)
        require(keys == english_keys, f"{name} keys do not match en.json")
        for key in required_keys:
            require(isinstance(translations[key], str) and translations[key].strip(),
                    f"{name} has an empty translation for {key!r}")


def validate_config() -> None:
    parser = configparser.ConfigParser(interpolation=None)
    parser.read(ROOT / "config" / "status-monitor" / "config.ini", encoding="utf-8")
    require(parser.getint("full", "refresh_rate") == 60, "Full refresh_rate must default to 60 Hz")

    widget_names = ("cpu", "gpu", "ram", "game", "temperatures", "system")
    require(parser.has_section("full-widgets"), "missing [full-widgets] configuration")
    for name in widget_names:
        for suffix in ("pinned", "x", "y", "scale"):
            require(parser.has_option("full-widgets", f"{name}_{suffix}"),
                    f"missing full-widgets option: {name}_{suffix}")


def validate_source() -> None:
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
    require(not missing_modes, f"missing mode sources: {', '.join(missing_modes)}")

    full = read_text(ROOT / "source" / "modes" / "Full.hpp")
    for marker in (
        "WidgetPlacement",
        "clampWidgetPlacement",
        "fullPanelHidden",
        "KEY_MINUS",
        "ult::currentTouchCount",
        "ult::currentTouchState",
        "saveWidget(",
        "drawRoundedRectSingleThreaded",
    ):
        require(marker in full, f"Full mode is missing implementation marker: {marker}")

    utils = read_text(ROOT / "source" / "Utils.hpp")
    for marker in (
        "EmaFilterState",
        "applyEma(",
        "isPlausibleTemperatureC",
        "acceptTemperatureC",
        "acceptTemperatureMilliC",
    ):
        require(marker in utils, f"telemetry safety is missing implementation marker: {marker}")

    tesla = read_text(ROOT / "lib" / "libryazhahand" / "libtesla" / "include" / "tesla.hpp")
    for marker in (
        "currentTouchCount.store",
        "currentTouchState = rawTouchState",
        "drawRect(447, 0, 1, 720",
    ):
        require(marker in tesla, f"libryazhahand integration is missing marker: {marker}")

    main = read_text(ROOT / "source" / "main.cpp")
    require('setupMode("full")' in main and '"-full"' in main,
            "Full mode must launch on its dedicated screen-wide layer")


def main() -> None:
    validate_metadata()
    validate_locales()
    validate_config()
    validate_source()
    print("Ryazha Status Monitor release validation passed.")


if __name__ == "__main__":
    main()
