# Building Ryazha Status Monitor

> **English — short:** use devkitPro with `switch-devkitA64`, `switch-tools`, and `switch-libnx`. Run `./scripts/test.sh` for host-side parser tests or `./scripts/build.sh` for the `.ovl` and complete ZIP package.

---

# Сборка Ryazha Status Monitor

## Требования

Для нативной сборки нужен установленный devkitPro с пакетами `switch-devkitA64`, `switch-tools` и `switch-libnx`. Переменная `DEVKITPRO` должна указывать на каталог установки; скрипт проверяет наличие `${DEVKITPRO}/libnx/switch_rules` до запуска сборки.

Для локальных проверок SMD-парсера нужны CMake 3.16+, компилятор C++23 и стандартные средства сборки. Эти тесты выполняются на компьютере разработчика и не требуют devkitPro.

## Проверка парсера и встроенных режимов

```bash
./scripts/test.sh
```

Сценарий создаёт отдельный каталог CMake, компилирует тесты со строгими предупреждениями и запускает CTest. Проверяются все встроенные `.smd`-режимы, сценарии настроек Full-режима, обработка команд рендеринга и рост памяти.

Для проверки с AddressSanitizer и UndefinedBehaviorSanitizer выполните:

```bash
cmake -S . -B build-sanitize -DSMD_SANITIZE=ON -DSMD_WARNINGS_AS_ERRORS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

Тест `test_memory_growth` намеренно отключается в санитайзерной конфигурации: санитайзеры меняют метаданные аллокатора и делают его измерения нерепрезентативными. Он запускается обычным набором `./scripts/test.sh`.

## Сборка релизного пакета

```bash
./scripts/build.sh
```

Скрипт очищает предыдущие результаты, запускает `make dist` и проверяет, что в архив попали оверлей и обязательная конфигурация. После успешной сборки создаются:

| Файл | Назначение |
| --- | --- |
| `Ryazha-Status-Monitor.ovl` | Оверлей для `sdmc:/switch/.overlays/` |
| `Ryazha-Status-Monitor.zip` | Полный архив для распаковки в корень SD-карты |
| `out/` | Временная структура содержимого архива |

## Сборка в контейнере

Dockerfile использует тот же образ `devkitpro/devkita64`, что и CI. Контейнерный вариант подходит, если devkitPro не установлен на хосте:

```bash
docker build -t ryazha-status-monitor .
docker run --rm ryazha-status-monitor
```

Контейнер создаёт полный дистрибутив. Он не публикует образы, теги или релизы и не требует паролей, токенов либо Docker Registry.

## Перед отправкой изменений

Перед публикацией выполните `./scripts/test.sh`, затем при наличии devkitPro — `./scripts/build.sh`. Проверьте, что полный ZIP содержит `switch/.overlays/Ryazha-Status-Monitor.ovl` и `config/status-monitor/locale.ini`. В GitHub Actions выполняются такие же проверки парсера и нативная сборка в devkitPro.
