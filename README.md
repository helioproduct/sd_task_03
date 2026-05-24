# REST API почтового сервиса

Простая реализация варианта 9 на `C++`, `userver` и `SQLite`. Решение специально сделано компактным: один HTTP-сервис, одна локальная база и минимальная структура проекта без разделения на несколько микросервисов.

## Что умеет сервис

- `POST /api/v1/users` - создать пользователя
- `POST /api/v1/auth/login` - получить токен
- `GET /api/v1/users/by-login?login=...` - найти пользователя по логину
- `GET /api/v1/users/search?mask=...` - найти пользователей по маске имени или фамилии
- `POST /api/v1/folders` - создать папку
- `GET /api/v1/folders` - получить список папок текущего пользователя
- `POST /api/v1/folders/{folder_id}/messages` - создать письмо в папке
- `GET /api/v1/folders/{folder_id}/messages` - получить письма из папки
- `GET /api/v1/messages/{message_id}` - получить письмо по идентификатору

## Технологии

- C++20
- userver
- SQLite
- Docker Compose
- pytest

## Структура проекта

```text
.
├── src/
│   ├── api/
│   ├── domain/
│   └── main.cpp
├── configs/
│   ├── static_config.yaml
│   └── dynamic_config_fallback.json
├── tests/
│   └── test_api.py
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yaml
├── openapi.yaml
├── QUICK_START.md
└── TASK_INFO.md
```

## Примеры запросов

Создание пользователя:

```bash
curl -X POST http://localhost:8080/api/v1/users \
  -H "Content-Type: application/json" \
  -d '{
    "login": "ivan",
    "email": "ivan@example.com",
    "first_name": "Ivan",
    "last_name": "Petrov",
    "password": "qwerty123"
  }'
```

Логин:

```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "login": "ivan",
    "password": "qwerty123"
  }'
```

## Тесты

Основной сценарий:

```bash
docker compose up --build -d
docker compose exec mail-lab pytest tests -v
```
