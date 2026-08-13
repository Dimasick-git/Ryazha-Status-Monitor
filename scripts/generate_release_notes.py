#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess
import re
import sys
from datetime import datetime

def get_version():
    """Извлекает версию из Makefile"""
    try:
        with open('Makefile', 'r', encoding='utf-8') as f:
            content = f.read()
            match = re.search(r'APP_VERSION\s*[:=]\s*([0-9.]+)', content)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Ошибка чтения Makefile: {e}")
    
    return "unknown"

def get_git_changes():
    """Получает изменения с последнего тэга"""
    try:
        # Получаем последний тэг
        result = subprocess.run(['git', 'describe', '--tags', '--abbrev=0'], 
                          capture_output=True, text=True)
        last_tag = result.stdout.strip() if result.returncode == 0 else None
        
        if last_tag:
            # Получаем изменения с последнего тэга
            result = subprocess.run(['git', 'log', '--pretty=format:- %s', f'{last_tag}..HEAD'], 
                              capture_output=True, text=True)
            changes = result.stdout.strip()
        else:
            # Все изменения если нет тэгов
            result = subprocess.run(['git', 'log', '--pretty=format:- %s'], 
                              capture_output=True, text=True)
            changes = result.stdout.strip()
        
        return changes
    except Exception as e:
        print(f"Ошибка получения изменений: {e}")
        return ""

def get_commit_stats():
    """Получает статистику коммитов"""
    try:
        result = subprocess.run(['git', 'rev-list', '--count', 'HEAD'], 
                          capture_output=True, text=True)
        commit_count = result.stdout.strip() if result.returncode == 0 else "0"
        
        result = subprocess.run(['git', 'log', '-1', '--format=%H'], 
                          capture_output=True, text=True)
        latest_commit = result.stdout.strip() if result.returncode == 0 else "unknown"
        
        return commit_count, latest_commit
    except Exception as e:
        print(f"Ошибка получения статистики: {e}")
        return "0", "unknown"

def generate_ai_style_notes(version, changes, commit_count, latest_commit):
    """Генерирует описание релиза в стиле ИИ"""
    
    # Анализируем изменения для категоризации
    changes_list = changes.split('\n') if changes else []
    
    categories = {
        'Исправления': [],
        'Новые функции': [],
        'Улучшения': [],
        'Обновления': [],
        'Другое': []
    }
    
    for change in changes_list:
        change = change.strip()
        if not change or not change.startswith('- '):
            continue
            
        change_text = change[2:].strip()  # Убираем "- "
        
        # Категоризируем изменения
        change_lower = change_text.lower()
        if any(word in change_lower for word in ['фикс', 'исправ', 'fix', 'bug', 'ошибка']):
            categories['Исправления'].append(change_text)
        elif any(word in change_lower for word in ['добав', 'нов', 'add', 'new', 'feature']):
            categories['Новые функции'].append(change_text)
        elif any(word in change_lower for word in ['улучш', 'оптим', 'improve', 'optimize']):
            categories['Улучшения'].append(change_text)
        elif any(word in change_lower for word in ['обнов', 'update', 'upgrade']):
            categories['Обновления'].append(change_text)
        else:
            categories['Другое'].append(change_text)
    
    # Генерируем описание
    notes = []
    
    # Заголовок
    notes.append(f"# Ryazha Status Monitor {version}")
    notes.append("")
    
    # Основное описание
    notes.append("## 📋 Описание релиза")
    notes.append("")
    notes.append(f"Выпуск Ryazha Status Monitor версии {version} с улучшениями и исправлениями.")
    notes.append("")
    
    # Категоризированные изменения
    if any(categories.values()):
        notes.append("## 🔄 Основные изменения")
        notes.append("")
        
        for category, items in categories.items():
            if items:
                notes.append(f"### {category}")
                for item in items:
                    notes.append(f"- {item}")
                notes.append("")
    
    # Техническая информация
    notes.append("## 🛠 Техническая информация")
    notes.append("")
    notes.append(f"- **Версия:** {version}")
    notes.append(f"- **Сборка:** {latest_commit[:8]}")
    notes.append(f"- **Коммитов:** {commit_count}")
    notes.append(f"- **Дата:** {datetime.now().strftime('%d.%m.%Y')}")
    notes.append("- **Платформа:** Nintendo Switch Homebrew")
    notes.append("")
    
    # Инструкция по установке
    notes.append("## 📦 Установка")
    notes.append("")
    notes.append("1. Скачайте файл `Ryazha-Status-Monitor.ovl`")
    notes.append("2. Поместите в папку `sdmc:/switch/.overlays/`")
    notes.append("3. Запустите через Overlay Loader")
    notes.append("")
    
    # Требования
    notes.append("## ⚠️ Системные требования")
    notes.append("")
    notes.append("- Установленная Atmosphère CFW")
    notes.append("- Overlay Loader для запуска оверлеев")
    notes.append("- Nintendo Switch с прошивкой 13.0.0+")
    notes.append("")
    
    # Футер
    notes.append("---")
    notes.append("")
    notes.append("*Релиз сгенерирован автоматически с использованием систем анализа изменений*")
    notes.append("")
    
    return '\n'.join(notes)

def main():
    """Основная функция"""
    output_file = sys.argv[1] if len(sys.argv) > 1 else 'release-notes.md'
    
    print("Генерация заметок релиза...")
    
    # Получаем информацию
    version = get_version()
    changes = get_git_changes()
    commit_count, latest_commit = get_commit_stats()
    
    print(f"Версия: {version}")
    print(f"Изменений: {len(changes.split()) if changes else 0}")
    
    # Генерируем заметки
    notes = generate_ai_style_notes(version, changes, commit_count, latest_commit)
    
    # Сохраняем в файл
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(notes)
        print(f"Заметки релиза сохранены в {output_file}")
    except Exception as e:
        print(f"Ошибка сохранения файла: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
