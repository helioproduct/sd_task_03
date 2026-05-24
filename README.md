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

## Запуск и проверка

### Запуск проекта

```bash
docker compose up --build -d
```

Сначала поднимется `postgres`, затем API-сервис `mail-lab`.

### Проверка статуса

```bash
docker compose ps
```

Оба контейнера должны быть в статусе `healthy`.

### Тестирование

Основной сценарий:

```bash
docker compose exec mail-lab pytest tests -v
```

### Swagger UI

Откройте в браузере: http://localhost:8080/swagger

## SQL-файлы

Проверить схему и данные можно так:

```bash
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "\dt"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from users;"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from folders;"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from messages;"
```

### Остановка

```bash
docker compose down
```

## Оптимизация запросов

### Что оптимизировалось

Основные запросы в сервисе:

1. поиск пользователя по логину
2. поиск пользователя по маске имени
3. получение папок пользователя
4. получение писем в папке
5. получение письма по идентификатору

### Индексы и зачем они нужны

- `users(login)` и `users(email)` создаются автоматически за счет `UNIQUE`
- `idx_users_full_name_lookup` ускоряет поиск по `mask`
- `idx_folders_owner_id` ускоряет выборку папок по `user_id`
- `idx_messages_folder_id` ускоряет фильтрацию писем по папке
- `idx_messages_sender_id` ускоряет связи с отправителем
- `idx_messages_folder_created_at` покрывает частый запрос `WHERE folder_id = ? ORDER BY created_at DESC`

### EXPLAIN до и после

#### Поиск пользователя по логину

До индекса:

```sql
EXPLAIN SELECT id, login, email FROM users WHERE login = 'alex.ivanov';
```

Типичный план:

```text
Seq Scan on users
  Filter: (login = 'alex.ivanov')
```

После индекса:

```text
Index Scan using users_login_key on users
  Index Cond: (login = 'alex.ivanov')
```

#### Получение папок пользователя

До индекса:

```text
Seq Scan on folders
  Filter: (user_id = 1)
```

После индекса `idx_folders_owner_id`:

```text
Index Scan using idx_folders_owner_id on folders
  Index Cond: (user_id = 1)
```

#### Получение писем в папке

До индекса:

```text
Seq Scan on messages
  Filter: (folder_id = 1)
  Sort Key: created_at DESC
```

После составного индекса:

```text
Index Scan using idx_messages_folder_created_at on messages
  Index Cond: (folder_id = 1)
```

Отдельная операция сортировки больше не нужна, потому что строки читаются уже в нужном порядке.

### Вывод

Для этой модели самый важный индекс - `idx_messages_folder_created_at`, потому что запрос списка писем выполняется чаще других и одновременно фильтрует по папке и сортирует по дате. Остальные индексы закрывают поиск пользователя и выборки по внешним ключам.
