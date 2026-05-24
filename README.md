# REST API почтового сервиса

Третья лабораторная для варианта 9. Основа осталась компактной: один HTTP-сервис на `userver`, но хранилище переведено на `PostgreSQL`, а рядом добавлены SQL-артефакты для схемы, данных и оптимизации.

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
- PostgreSQL 16
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
├── db/
│   └── Dockerfile
├── tests/
│   └── test_api.py
├── CMakeLists.txt
├── Dockerfile
├── data.sql
├── docker-compose.yaml
├── optimization.md
├── openapi.yaml
├── queries.sql
├── schema.sql
└── TASK_INFO.md
```

## Что добавлено в lab 3

- `schema.sql` с таблицами, внешними ключами, `CHECK` и индексами
- `data.sql` с тестовыми записями
- `queries.sql` с SQL для всех операций API
- `optimization.md` с пояснением по индексам и `EXPLAIN`
- контейнер `postgres`, к которому подключается API

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

## SQL-файлы

Проверить схему и данные можно так:

```bash
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "\dt"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from users;"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from folders;"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from messages;"
```
