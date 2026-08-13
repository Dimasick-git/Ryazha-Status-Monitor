#!/bin/bash

# Скрипт для автоматического создания тэга при изменении APP_VERSION

# Получаем текущую версию из Makefile
CURRENT_VERSION=$(grep "APP_VERSION" Makefile | sed 's/APP_VERSION\s*:=\s*//' | tr -d '[:space:]')

# Получаем последний тэг
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")

# Проверяем, изменилась ли версия
if [ "$CURRENT_VERSION" != "$LAST_TAG" ]; then
    echo "Новая версия: $CURRENT_VERSION"
    echo "Последний тэг: $LAST_TAG"
    
    # Создаем и пушим новый тэг
    git config --local user.email "action@github.com"
    git config --local user.name "GitHub Action"
    git add Makefile
    git commit -m "Update version to $CURRENT_VERSION" || true
    git tag -a "v$CURRENT_VERSION" -m "Release version $CURRENT_VERSION"
    git push origin main --follow-tags
    
    echo "Создан тэг v$CURRENT_VERSION"
else
    echo "Версия не изменилась: $CURRENT_VERSION"
fi
