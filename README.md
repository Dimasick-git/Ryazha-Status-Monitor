# Ryazha Status Monitor

> **English — short:** a configurable Nintendo Switch monitoring overlay built on [Status-Monitor-Deux](https://github.com/masagrator/Status-Monitor-Deux). Download the full release ZIP, extract it to the root of the SD card, then open it from Ryazhahand, Ultrahand, or Tesla Menu. The ZIP contains both the overlay and the required modes, extensions, and locale files.

| Topic | English summary |
| --- | --- |
| Current release | **1.4.8** |
| Runtime | Atmosphère CFW with Ryazhahand, Ultrahand, or Tesla Menu |
| Installation | Extract `Ryazha-Status-Monitor.zip` to the SD-card root; do not install only the `.ovl` file |
| Documentation | [Build guide](docs/BUILDING.md), [Security](docs/SECURITY.md), [SMD format](docs/SMD_FORMAT.md), [parser internals](docs/SMD_PARSER_INTERNALS.md) |

---

# Русский

**Ryazha Status Monitor** — оверлей для Nintendo Switch, который показывает состояние системы и игры в реальном времени. Форк основан на [Status-Monitor-Deux](https://github.com/masagrator/Status-Monitor-Deux): режимы описываются файлами `.smd`, поэтому их можно настраивать и дополнять без перекомпиляции приложения.

## Установка

Скачайте **полный** архив `Ryazha-Status-Monitor.zip` со страницы [релизов](https://github.com/Dimasick-git/Ryazha-Status-Monitor/releases) и распакуйте его в корень SD-карты. В результате должны появиться `sdmc:/switch/.overlays/Ryazha-Status-Monitor.ovl` и `sdmc:/config/status-monitor/`.

> Не копируйте только файл `.ovl`: без папки `config/status-monitor` меню режимов будет пустым.

Для запуска нужен Atmosphère CFW и одно из совместимых меню оверлеев: **Ryazhahand**, **Ultrahand** или **Tesla Menu**. Оверлей не поддерживает SX OS и другие CFW.

## Что показывает оверлей

| Раздел | Данные |
| --- | --- |
| CPU, GPU и RAM | Загрузка, целевые и фактические частоты, использование памяти |
| Температуры и питание | Температуры SoC, PCB и корпуса, вентилятор, заряд, ток, напряжение и мощность батареи |
| Игра | PFPS, FPS, разрешения рендеринга и скорость чтения при установленном совместимом SaltyNX |
| Сеть и мультимедиа | Тип подключения, Wi-Fi-пароль по удержанию `Y`, частоты NVDEC, NVENC и NVJPG |

Фактические частоты и загрузка RAM доступны при работающем `sys-clk` или `hoc:clk`. Игровая телеметрия зависит от SaltyNX. Некоторые показания зависят от версии системного ПО и доступности сервисов Switch.

## Режимы и настройка

В архив входят Полный, Mini, Micro, график FPS, счётчик FPS, RyazhaStatus, статистика батареи/зарядки, системная информация и разрешения игры. Символ `Y` рядом с режимом означает настройки режима; глобальные настройки открываются в главном меню кнопкой влево на крестовине.

Перемещаемые режимы можно перетаскивать сенсором или гироскопом. Масштаб изменяется щипком двумя пальцами и запоминается отдельно для каждого режима. Комбинация `L + R + ВВЕРХ` по умолчанию открывает Полный режим из меню и закрывает его внутри режима; её можно изменить в `config.ini`.

Режимы используют цвета Ryazhahand из `/config/ryazhahand/theme.ini`. При отсутствии этого файла используется `/config/ultrahand/theme.ini`. В режимах, где это поддержано, включите «Цвета из темы» в настройках режима.

## Языки

Основные языки интерфейса — русский, английский США, польский и немецкий. Язык выбирается из настройки `default_lang` Ryazhahand, затем из языка системы. Если подходящего перевода нет, используется русский. Все пользовательские настройки встроенных SMD-режимов переведены для доступных языков.

## Собственные режимы и расширения

Пользовательские режимы `.smd` размещайте в `sdmc:/config/status-monitor/modes/`, а расширения `.smse` — в `sdmc:/config/status-monitor/extensions/`. Формат режимов описан в [справочнике SMD](docs/SMD_FORMAT.md); устройство реализации — в [документации парсера](docs/SMD_PARSER_INTERNALS.md). Расширение для VS Code находится в `.vscode/extensions/statusmonitor-smd`.

Старые аргументы запуска `-mini`, `-micro`, `-full`, `-fps_graph`, `-fps_counter`, `-game_resolutions`, `--microOverlay` и `--microOverlay_` сохранены для совместимости. Новый штатный вариант — `--file <имя_режима>.smd`.

## Сборка и проверка

Для разработчиков доступна краткая инструкция в [docs/BUILDING.md](docs/BUILDING.md); границы доверия и ограничения входных файлов описаны в [docs/SECURITY.md](docs/SECURITY.md). Локальная проверка парсера запускается командой `./scripts/test.sh`. Сборка полного пакета выполняется командой `./scripts/build.sh` в среде devkitPro; результатом будут `.ovl` и архив `Ryazha-Status-Monitor.zip`.

## Устранение неполадок

| Симптом | Что проверить |
| --- | --- |
| Список режимов пуст | Убедитесь, что распакован полный ZIP и существует `sdmc:/config/status-monitor/modes/` |
| Нет данных об игре | Проверьте, что игра запущена и установлен совместимый SaltyNX |
| Частоты не меняются | Проверьте установленный и запущенный `sys-clk` или `hoc:clk` |
| Оверлей нестабилен при подключении к сети | Отключите конфликтующие патчи `nifm`/`ctest` и ненужные непроверенные sysmodule, затем перезагрузите Switch |

## Благодарности

Спасибо MasaGratoR за Status Monitor Overlay и Status-Monitor-Deux, RetroNX за помощь с разработкой, SunTheCourier за исходные материалы Tesla, Herbaciarz за тестирование со скриншотами HDMI Grabber, KazushiMe и CTCaer за материалы по контроллеру батареи, ChanseyIsTheBest за тестирование разрешений игр и Lightos за немецкий перевод.

## Лицензия

Проект распространяется по [GPL-2.0](LICENSE).
