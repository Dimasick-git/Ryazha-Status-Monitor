#!/usr/bin/env python3
"""Apply Ryazhenka's compact Russian terminology to the ppkantorski locale."""
from __future__ import annotations

import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_LOCALE = ROOT / "lang" / "ru.json"
INSTALL_LOCALE = ROOT / "config" / "status-monitor" / "lang" / "ru.json"

# Values shown in a live compact mode must stay short.  Technical FPS remains
# universally recognisable; Russian hardware labels use no more than two letters.
REPLACEMENTS = {
    "Status Monitor": "Ряжа-Монитор",
    "Modes   Configure": "Режимы   Настройки",
    "FPS Graph": "FPS-график",
    "FPS Counter": "FPS + Гц",
    "Game Resolutions": "Разрешение",
    "Battery/Charger": "Аккум./зарядка",
    "Miscellaneous": "Разное",
    "CPU Usage": "Загр. ЦП",
    "GPU Usage": "Загр. ГП",
    "RAM Usage": "Загр. ОЗ",
    "Target Frequency": "Цел. частота",
    "Real Frequency": "Реал. частота",
    "Load": "Загр.",
    "Board": "ПП",
    "Battery Power Flow": "Питание АК",
    "SoC\nPCB\nSkin": "СЧ\nПП\nК",
    "Resolutions  ": "Разр. ",
    "Read Speed  ": "Чтение ",
    "Viewport": "Область",
    "Battery Stats": "Данные АК",
    "Actual Capacity": "Факт. ёмкость",
    "Designed Capacity": "Номин. ёмкость",
    "Raw Charge": "Сырой заряд",
    "Current Flow": "Ток",
    "Power Flow": "Мощность",
    "Remaining Time": "Осталось",
    "Charger Stats": "Зарядка",
    "Input Current Limit": "Лимит вход. тока",
    "VBUS Current Limit": "Лимит VBUS",
    "Voltage Limit": "Лимит напр.",
    "Current Limit": "Лимит тока",
    "Max Voltage": "Макс. напр.",
    "Max Current": "Макс. ток",
    "Multimedia Clock Rates": "Частоты медиа",
    "Network": "Сеть",
    "Type: Wi-Fi": "Тип: Wi‑Fi",
    "Type: Ethernet": "Тип: Ethernet",
    "Type: Not connected": "Тип: нет сети",
    "CPU\nGPU\nRAM\nSOC\nPCB\nSKN": "ЦП\nГП\nОЗ\nСЧ\nПП\nК",
    "Elements   Move Down   Move Up": "Элементы   Вниз   Вверх",
    "FileSafe": "Безопасно",
    "DTC Format": "Формат ВР",
    "Info": "Инфо",
    "Disable Screenshots": "Без скриншотов",
    "Real Freqs": "Реал. част.",
    "Deltas": "Откл.",
    "Target Freqs": "Цел. част.",
    "RES": "Рз",
    "Read Speed": "Чтение",
    "Real Frequencies": "Реал. част.",
    "Real Voltages": "Реал. напр.",
    "Full CPU": "ЦП: все ядра",
    "Full Resolution": "Полн. разр.",
    "SOC Voltage": "Напр. СЧ",
    "RAM Load CPU/GPU": "ОЗ: ЦП/ГП",
    "Use DTC Symbol": "Значок ВР",
    "Use Dynamic Colors": "Дин. цвета",
    "Configuration": "Настройки",
    "Refresh Rate": "Обновление",
    "Frame Padding": "Отступ рамки",
    "Handheld Font Size": "Шрифт: портативный",
    "Docked Font Size": "Шрифт: док",
    "FPS Counter Color": "Цвет FPS",
    "FPS Counter Alpha": "Прозрачность FPS",
    "CPU": "ЦП",
    "GPU": "ГП",
    "RAM": "ОЗ",
    "SOC": "СЧ",
    "PCB": "ПП",
    "TMP": "Т",
    "BAT": "АК",
    "DTC": "ВР",
    "Skin": "К",
    "CPU  ": "ЦП  ",
    "GPU  ": "ГП  ",
    "MEM  ": "ПМ  ",
    "SOC  ": "СЧ  ",
    "PCB  ": "ПП  ",
    "Skin  ": "К  ",
    "Touch Move Delay": "Задержка касания",
    "Button Move Delay": "Задержка кнопок",
    "Mode Combo": "Комб. режима",
    "DTC Format 1": "Формат ВР 1",
    "DTC Format 2": "Формат ВР 2",
    "Target Frequencies": "Цел. частоты",
    "Frequency Deltas": "Откл. частот",
    "Integer FPS": "FPS без дроби",
    "Use FPS Graph": "График FPS",
    "Dynamic Temps": "Дин. Т",
    "Stacked Temps": "Т столб.",
    "CPU/GPU/RAM Temps": "Т ЦП/ГП/ОЗ",
    "SOC/PCB/Skin Temps": "Т СЧ/ПП/К",
    "CPU Temp": "Т ЦП",
    "GPU Temp": "Т ГП",
    "RAM Temp": "Т ОЗ",
    "Stacked CPU Temp": "Т ЦП, столб.",
    "Stacked GPU Temp": "Т ГП, столб.",
    "Stacked RAM Temp": "Т ОЗ, столб.",
    "Stacked Full CPU": "ЦП, столб.",
    "Full CPU Max Core 0-2": "ЦП: ядра 0–2",
    "RAM Bandwidth": "ПСП ОЗ",
    "Stacked RAM Bandwidth": "ПСП ОЗ, столб.",
    "Voltage At End": "Напр. в конце",
    "Primary Only": "Только осн.",
}


ADDITIONAL_TRANSLATIONS = {
    "Show Refresh Hz": "Показывать Гц",
    "EMA Smoothing": "EMA-сглаживание",
    "EMA FPS": "EMA: FPS",
    "EMA Frequencies": "EMA: частоты",
    "EMA Loads": "EMA: загрузка",
    "EMA Temperatures": "EMA: температура",
    "EMA Power": "EMA: питание",
    "Use Ryazha Theme": "Тема Ряженки",
}


def main() -> None:
    with SOURCE_LOCALE.open("r", encoding="utf-8") as file:
        locale = json.load(file)

    missing = sorted(key for key in REPLACEMENTS if key not in locale)
    if missing:
        raise SystemExit("Missing upstream locale keys: " + ", ".join(missing))

    locale.update(REPLACEMENTS)
    locale.update(ADDITIONAL_TRANSLATIONS)
    with SOURCE_LOCALE.open("w", encoding="utf-8", newline="\n") as file:
        json.dump(locale, file, ensure_ascii=False, indent=4)
        file.write("\n")

    INSTALL_LOCALE.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(SOURCE_LOCALE, INSTALL_LOCALE)
    print(f"Updated {len(REPLACEMENTS) + len(ADDITIONAL_TRANSLATIONS)} Russian labels and staged the install locale.")


if __name__ == "__main__":
    main()
