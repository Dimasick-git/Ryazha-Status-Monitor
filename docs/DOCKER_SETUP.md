# Docker Registry Setup Guide

## 🐳 Настройка Docker Registry для Ryazha-Status-Monitor

### 📋 Требования:
- Docker Hub аккаунт
- GitHub репозиторий
- Права на настройку Secrets

### 🔧 Шаги настройки:

#### 1️⃣ **Настройка GitHub Secrets:**

Перейдите в ваш репозиторий GitHub: `https://github.com/Dimasick-git/Ryazha-Status-Monitor/settings/secrets`

Добавьте следующие Secrets:

```
DOCKER_USERNAME=dimasickgit880312
DOCKER_PASSWORD=ваш_пароль_от_Docker_Hub
```

#### 2️⃣ **Настройка Docker Hub:**

1. Войдите в [Docker Hub](https://hub.docker.com/)
2. Проверьте ваш username: `dimasickgit880312`
3. Создайте репозиторий `ryazha-status-monitor`
4. Убедитесь, что пароль правильный

#### 3️⃣ **Автоматическая сборка:**

После настройки Secrets CI/CD pipeline будет автоматически:
- ✅ Собирать Docker образы при push в main
- ✅ Публиковать образы в Docker Hub
- ✅ Создавать версии по тегам
- ✅ Проводить тестирование образов

### 🚀 **Использование Docker образов:**

#### Сборка локально:
```bash
# Использовать Docker Hub образ
docker pull dimasickgit880312/ryazha-status-monitor:latest

# Запустить контейнер
docker run --rm -v $(pwd):/workspace dimasickgit880312/ryazha-status-monitor:latest

# Сборка в Docker
docker build -t ryazha-status-monitor .
```

#### Разработка:
```bash
# Использовать docker-compose
docker-compose up dev

# Сборка
docker-compose up build

# Тестирование
docker-compose up test
```

### 📊 **CI/CD Pipeline:**

#### Триггеры:
- **Push в main** → сборка и публикация `:latest`
- **Push тега** → сборка и публикация версии
- **Pull Request** → тестирование без публикации

#### Артефакты:
- Docker образы в GitHub Container Registry
- Docker образы в Docker Hub
- Автоматические тесты
- Метаданные и теги

### 🔍 **Тестирование:**

#### Локальное тестирование:
```bash
# Тест сборки
docker-compose up test

# Проверка образа
docker run --rm dimasickgit880312/ryazha-status-monitor:latest ls -la

# Тест компиляции
docker run --rm -v $(pwd):/workspace dimasickgit880312/ryazha-status-monitor:latest make test
```

#### CI тестирование:
- Автоматическая проверка работоспособности
- Тестирование артефактов
- Валидация Docker образа

### 🐛 **Troubleshooting:**

#### Ошибка "Access denied":
```bash
# Проверьте credentials
docker login -u dimasickgit880312

# Проверьте Secrets в GitHub
# Убедитесь, что DOCKER_PASSWORD правильный
```

#### Ошибка "Repository not found":
```bash
# Создайте репозиторий в Docker Hub
docker repo create ryazha-status-monitor
```

#### Ошибка сборки:
```bash
# Проверьте Dockerfile
docker build -t test .

# Локальная отладка
docker run --rm -it test bash
```

### 📝 **Примеры использования:**

#### Разработка с Docker:
```bash
# Запуск dev окружения
docker-compose up dev

# Сборка проекта
docker-compose up build

# Отправка в registry
docker-compose up release
```

#### Production:
```bash
# Pull последней версии
docker pull dimasickgit880312/ryazha-status-monitor:latest

# Запуск с volume
docker run -d \
  --name ryazha-monitor \
  -v /path/to/config:/workspace/config \
  dimasickgit880312/ryazha-status-monitor:latest
```

### 🔄 **Автоматизация:**

#### GitHub Actions:
- 🔄 Автоматическая сборка при изменениях
- 📦 Автоматическая публикация
- 🧪 Автоматическое тестирование
- 📊 Автоматическая генерация метаданных

#### Теги версий:
- `v1.0.0` → `dimasickgit880312/ryazha-status-monitor:1.0.0`
- `v1.0` → `dimasickgit880312/ryazha-status-monitor:1.0`
- `v1` → `dimasickgit880312/ryazha-status-monitor:1`
- `main` → `dimasickgit880312/ryazha-status-monitor:latest`

### 📈 **Мониторинг:**

#### Метрики сборки:
- Размер образов
- Время сборки
- Количество слоев
- Безопасность

#### Статистика:
- Скачивания образов
- Использование в CI/CD
- Тестирование производительности

---

**После настройки Docker registry ваш проект будет полностью автоматизирован!** 🚀
