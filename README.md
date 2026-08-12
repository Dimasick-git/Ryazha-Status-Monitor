# Ryazha Status Monitor

> **English — short:** Ryazha Status Monitor is a configurable Nintendo Switch monitoring overlay based on [Status-Monitor-Deux](https://github.com/masagrator/Status-Monitor-Deux). It works with Atmosphère CFW and Ryazhahand, Ultrahand, or Tesla Menu. Download the latest full ZIP from [Releases](https://github.com/Dimasick-git/Ryazha-Status-Monitor/releases), extract it to the SD-card root, and open the overlay from your menu.

| Topic | English summary |
| --- | --- |
| What it does | Shows CPU, GPU, RAM, temperature, battery, FPS, game resolution, network, and system telemetry. |
| Installation | Extract the **full ZIP** to the SD-card root. Do not install only the `.ovl` file. |
| Customization | Built-in `.smd` modes, theme support, translations, custom modes, and SMSE extensions. |
| Documentation | [Security](docs/SECURITY.md), [Build guide](docs/BUILDING.md), [SMD reference](docs/SMD_FORMAT.md), [Releases](https://github.com/Dimasick-git/Ryazha-Status-Monitor/releases). |

---

# Русская документация

## О проекте

**Ryazha Status Monitor** — это настраиваемый мониторинг-оверлей для Nintendo Switch. Он показывает состояние консоли и игры поверх запущенного приложения, а внешний вид и состав данных определяются готовыми или пользовательскими режимами `.smd`.

Проект основан на [Status-Monitor-Deux](https://github.com/masagrator/Status-Monitor-Deux) и адаптирован для экосистемы Ryazhahand. В нём сохранена совместимость с Ryazhahand, Ultrahand и Tesla Menu, добавлены русская локализация, темы Ryazhahand, современные глифы кнопок и расширенные встроенные режимы.

## Быстрый старт

1. Откройте страницу [релизов](https://github.com/Dimasick-git/Ryazha-Status-Monitor/releases) и скачайте полный архив `Ryazha-Status-Monitor.zip`.
2. Распакуйте архив **в корень SD-карты** с заменой файлов.
3. Проверьте, что появились `sdmc:/switch/.overlays/Ryazha-Status-Monitor.ovl` и папка `sdmc:/config/status-monitor/`.
4. Откройте оверлей через Ryazhahand, Ultrahand или Tesla Menu и выберите режим.

> Не копируйте на карту только файл `.ovl`. Без папки `config/status-monitor` не будут доступны встроенные режимы, локализация и расширения.

## Требования и совместимость

Для работы требуется Nintendo Switch с Atmosphère CFW и одним из поддерживаемых меню оверлеев: **Ryazhahand**, **Ultrahand** или **Tesla Menu**. SX OS и другие CFW не поддерживаются.

Часть телеметрии зависит от установленного окружения. Фактические частоты и нагрузка RAM доступны при работающем `sys-clk` или `hoc:clk`; игровые FPS и разрешения требуют совместимый SaltyNX. Набор отдельных значений также зависит от версии системного ПО и доступности сервисов Switch.

## Что умеет оверлей

| Раздел | Данные |
| --- | --- |
| Производительность | Нагрузка и частоты CPU/GPU/RAM, целевые и фактические частоты, использование памяти. |
| Температуры и питание | Температуры SoC, PCB и корпуса, вентилятор, заряд, ток, напряжение, мощность и прогноз времени работы батареи. |
| Игра | FPS/PFPS, сглаженный FPS, скорость чтения, разрешения рендеринга и viewport. |
| Система | Время, состояние док-станции, сеть, Wi-Fi, мультимедийные частоты NVDEC/NVENC/NVJPG. |
| Оформление | Перемещение, масштаб сенсором или гироскопом, темы Ryazhahand, локализация и современные глифы подсказок кнопок. |

## Встроенные режимы

| Режим | Назначение |
| --- | --- |
| Full | Полный набор системных и игровых показателей с гибкими настройками строк. |
| Mini | Компактная сводка производительности для игры. |
| Micro | Минимальный постоянно видимый индикатор. |
| FPS Graph | График FPS с историей значений. |
| FPS Counter | Отдельный крупный счётчик FPS. |
| RyazhaStatus | FPS и частота дисплея со сглаживанием и цветовой индикацией. |
| Battery/Charger | Подробная информация о батарее и зарядном устройстве. |
| Miscellaneous | Состояние сети и мультимедийные частоты. |
| Game Resolutions | Разрешения рендеринга и viewport запущенной игры. |

Нажмите `Y` на выбранном режиме, чтобы открыть его параметры. Глобальные настройки доступны из главного меню кнопкой влево на крестовине. По умолчанию комбинация `L + R + ВВЕРХ` открывает Full-режим из меню и закрывает его внутри режима; её можно изменить в `config.ini`.

## Оформление, управление и темы

Перемещаемые режимы можно передвигать сенсорным экраном или гироскопом. Масштаб изменяется щипком двумя пальцами и сохраняется отдельно для каждого режима. Подсказки комбинаций используют карту глифов **Switch 2 Style** из libryazhahand: поддерживаются `A`, `B`, `X`, `Y`, `L`, `R`, `ZL`, `ZR`, `SL`, `SR`, `LS`, `RS`, `Plus`, `Minus` и крестовина.

По умолчанию режимы читают цвета из `/config/ryazhahand/theme.ini`; при отсутствии файла используется `/config/ultrahand/theme.ini`. В поддерживаемых режимах включите параметр «Цвета из темы», чтобы применить активную тему.

## Языки

Встроенные режимы переведены на русский, английский США, польский и немецкий. Язык выбирается из `default_lang` Ryazhahand, затем из языка системы; при отсутствии подходящего перевода используется русский.

## Собственные режимы и расширения

Пользовательские режимы `.smd` размещайте в `sdmc:/config/status-monitor/modes/`, а расширения `.smse` — в `sdmc:/config/status-monitor/extensions/`. Формат режимов описан в [справочнике SMD](docs/SMD_FORMAT.md), а архитектура парсера — в [документации для разработчиков](docs/SMD_PARSER_INTERNALS.md). Расширение подсветки синтаксиса VS Code находится в `.vscode/extensions/statusmonitor-smd`.

Старые аргументы `-mini`, `-micro`, `-full`, `-fps_graph`, `-fps_counter`, `-game_resolutions`, `--microOverlay` и `--microOverlay_` сохранены для совместимости. Предпочтительный вариант запуска пользовательского режима — `--file <имя_режима>.smd`.

> Устанавливайте `.smse`-расширения только из доверенных источников: они могут выполнять описанные ими IPC-запросы к сервисам Switch. Полная модель доверия, лимиты входных данных и рекомендации опубликованы в [docs/SECURITY.md](docs/SECURITY.md).

## Обновление

При обновлении скачивайте и распаковывайте полный ZIP-архив поверх существующей установки. Собственные файлы лучше предварительно сохранить отдельно: особенно `config.ini`, созданные режимы `.smd` и расширения `.smse`.

## Устранение неполадок

| Симптом | Что проверить |
| --- | --- |
| Список режимов пуст | Убедитесь, что распакован полный ZIP и существует `sdmc:/config/status-monitor/modes/`. |
| Нет игровых данных | Проверьте, что игра запущена и установлен совместимый SaltyNX. |
| Частоты не меняются | Проверьте установленный и активный `sys-clk` или `hoc:clk`. |
| Тема не применяется | Проверьте путь к `theme.ini` и включите «Цвета из темы» в настройках режима. |
| Нестабильность при сети | Отключите конфликтующие патчи `nifm`/`ctest` и непроверенные sysmodule, затем перезагрузите Switch. |

## Разработка

Для локальных проверок SMD-парсера выполните:

```bash
./scripts/test.sh
```

Для сборки полного дистрибутива в среде devkitPro выполните:

```bash
./scripts/build.sh
```

Подробности доступны в [руководстве по сборке](docs/BUILDING.md). Релизные заметки не дублируются в README: они публикуются отдельно для каждой версии в [Releases](https://github.com/Dimasick-git/Ryazha-Status-Monitor/releases).

## Благодарности

Спасибо MasaGratoR за Status Monitor Overlay и Status-Monitor-Deux, RetroNX за помощь с разработкой, SunTheCourier за исходные материалы Tesla, Herbaciarz за тестирование со скриншотами HDMI Grabber, KazushiMe и CTCaer за материалы по контроллеру батареи, ChanseyIsTheBest за тестирование разрешений игр и Lightos за немецкий перевод.

## Лицензия

Проект распространяется по [GPL-2.0](LICENSE).
