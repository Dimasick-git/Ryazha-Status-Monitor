#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODES = ROOT / "modes"


def append_once(path: Path, lines: list[str]) -> None:
    source = path.read_text(encoding="utf-8")
    additions = [line for line in lines if line not in source]
    if additions:
        path.write_text(source.rstrip() + "\n\n" + "\n".join(additions) + "\n", encoding="utf-8")


append_once(MODES / "01.Full.smd", [
    'NAME_LOCALE{"RU-RU", "Полный"}',
    'IETF_LOCALE{ComboButtonFormat, "RU-RU", "Удерживайте %s для выхода"}',
    'IETF_LOCALE{TargetFreqTextFormat, "RU-RU", "Целевая частота: %u.%u МГц"}',
    'IETF_LOCALE{RealFreqTextFormat, "RU-RU", "Текущая частота: %u.%u МГц"}',
    'IETF_LOCALE{CPULoadTextFormat, "RU-RU", "Ядро #0: %.2lf%%\\nЯдро #1: %.2lf%%\\nЯдро #2: %.2lf%%\\nЯдро #3: %.2lf%%"}',
    'IETF_LOCALE{GPULoadTextFormat, "RU-RU", "Нагрузка: %u.%u%%"}',
    'IETF_LOCALE{RAMLoadTextFormat, "RU-RU", "Нагрузка: %u.%u%% (CPU %u.%u | GPU %u.%u)"}',
    'IETF_LOCALE{RAMUsageBaseTextFormat, "RU-RU", "Всего: \\nПриложение: \\nАпплет: \\nСистема: \\nСистема (небезопасная): "}',
    'IETF_LOCALE{BatteryChargingTextFormat, "RU-RU", "Поток питания батареи: %+.2f Вт [%s]"}',
    'IETF_LOCALE{FanRotationTextFormat, "RU-RU", "Скорость вентилятора: %2.1f%%"}',
    'IETF_LOCALE{FPSNDText, "RU-RU", "PFPS: н/д; FPS: н/д"}',
    'IETF_LOCALE{ReadSpeedTextFormat, "RU-RU", "Скорость чтения: %.2f МиБ/с"}',
    'IETF_LOCALE{ReadSpeedUnkTextFormat, "RU-RU", "Скорость чтения: н/д"}',
    'IETF_LOCALE{CpuUsageText, "RU-RU", "Нагрузка CPU:"}',
    'IETF_LOCALE{GpuUsageText, "RU-RU", "Нагрузка GPU:"}',
    'IETF_LOCALE{RamUsageText, "RU-RU", "Использование RAM:"}',
    'IETF_LOCALE{BoardUsageText, "RU-RU", "Плата:"}',
    'IETF_LOCALE{TemperatureText, "RU-RU", "Температуры:"}',
    'IETF_LOCALE{ThermalSensorsText, "RU-RU", "SOC \\nPCB \\nКорпус "}',
    'IETF_LOCALE{GameText, "RU-RU", "Игра:"}',
    'IETF_LOCALE{Resolutions1Text, "RU-RU", "Разрешение: %dx%d"}',
    'IETF_LOCALE{Resolutions2Text, "RU-RU", "Разрешения: %dx%d || %dx%d"}',
])

append_once(MODES / "02.Mini.smd", [
    'NAME_LOCALE{"RU-RU", "Мини"}',
    'IETF_LOCALE{DrawTextCat, "RU-RU", "РЕНД"}',
    'IETF_LOCALE{FanTextCat, "RU-RU", "ВЕНТ"}',
    'IETF_LOCALE{ResTextCat, "RU-RU", "РАЗР"}',
    'IETF_LOCALE{ReadTextCat, "RU-RU", "ЧТЕН"}',
])

append_once(MODES / "03.Micro.smd", [
    'NAME_LOCALE{"RU-RU", "Микро"}',
    'IETF_LOCALE{BoardTextCat, "RU-RU", "ПЛАТ"}',
    'IETF_LOCALE{FanTextCat, "RU-RU", "ВЕНТ"}',
])

append_once(MODES / "FPS/01.FPSGraph.smd", [
    'NAME_LOCALE{"RU-RU", "График FPS"}',
])

append_once(MODES / "FPS/02.FPSCounter.smd", [
    'NAME_LOCALE{"RU-RU", "Счётчик FPS"}',
])

append_once(MODES / "Other/01.BatteryCharger.smd", [
    'NAME_LOCALE{"RU-RU", "Батарея и зарядка"}',
    'IETF_LOCALE{Text01Format, "RU-RU", "Фактическая ёмкость батареи: %.0f мА·ч\\n"}',
    'IETF_LOCALE{Text02Format, "RU-RU", "Расчётная ёмкость батареи: %.0f мА·ч\\n"}',
    'IETF_LOCALE{Text03Format, "RU-RU", "Температура батареи: %.1f°C\\n"}',
    'IETF_LOCALE{Text04Format, "RU-RU", "Сырой заряд батареи: %.1f%%\\n"}',
    'IETF_LOCALE{Text05Format, "RU-RU", "Износ батареи: %.1f%%\\n"}',
    'IETF_LOCALE{Text06Format, "RU-RU", "Напряжение батареи (%ds среднее): %.0f мВ\\n"}',
    'IETF_LOCALE{Text07Format, "RU-RU", "Ток батареи (%ss среднее): %+.0f мА\\n"}',
    'IETF_LOCALE{Text08Format, "RU-RU", "Поток питания батареи%s: %+.3f Вт\\n"}',
    'IETF_LOCALE{Text09Format, "RU-RU", "Осталось работы батареи: %s\\n"}',
    'IETF_LOCALE{Text10Format, "RU-RU", "Тип зарядного устройства: %u\\n"}',
    'IETF_LOCALE{Text11Format, "RU-RU", "Максимальное напряжение ЗУ: %u мВ\\n"}',
    'IETF_LOCALE{Text12Format, "RU-RU", "Максимальный ток ЗУ: %u мА\\n"}',
    'IETF_LOCALE{Avg5s, "RU-RU", " (среднее за 5 с)"}',
    'IETF_LOCALE{Title, "RU-RU", "Состояние батареи и зарядки:"}',
])

append_once(MODES / "Other/02.Miscellaneous.smd", [
    'NAME_LOCALE{"RU-RU", "Разное"}',
    'IETF_LOCALE{MultimediaText, "RU-RU", "Частоты мультимедиа:"}',
    'IETF_LOCALE{NetworkText, "RU-RU", "Сеть:"}',
    'IETF_LOCALE{WiFiText, "RU-RU", "Тип: Wi‑Fi"}',
    'IETF_LOCALE{ShowPasswordText, "RU-RU", "Нажмите Y, чтобы показать пароль"}',
    'IETF_LOCALE{EthernetText, "RU-RU", "Тип: Ethernet"}',
    'IETF_LOCALE{NotConnectedText, "RU-RU", "Тип: нет подключения"}',
])

append_once(MODES / "Other/03.GameResolutions.smd", [
    'NAME_LOCALE{"RU-RU", "Разрешения игры"}',
    'IETF_LOCALE{WarningText, "RU-RU", "Игра не запущена или несовместима."}',
])

folder = MODES / "Other/_folder.ini"
source = folder.read_text(encoding="utf-8")
if "RU-RU=" not in source:
    folder.write_text(source.rstrip() + "\nRU-RU=Другое\n", encoding="utf-8")

print("Russian mode localizations added.")
