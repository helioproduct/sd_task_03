# Лабораторная работа 3. Проектирование и оптимизация реляционной БД

Вариант 9: электронная почта.

В работе спроектирована реляционная база данных для почтового сервиса. Основные сущности:

- пользователь
- почтовая папка
- сообщение

## Обязательные файлы

- `schema.sql` - создание схемы БД, ограничений и индексов
- `data.sql` - тестовые данные
- `queries.sql` - SQL-запросы для операций варианта
- `optimization.md` - описание оптимизаций и планы выполнения `EXPLAIN`
- `README.md` - описание схемы и инструкция по запуску

## Структура БД

### `users`

Хранит пользователей системы.

Поля:

- `id` - первичный ключ
- `login` - уникальный логин
- `email` - уникальный email
- `first_name` - имя
- `last_name` - фамилия
- `password_hash` - хэш пароля
- `created_at` - дата создания записи

Ограничения:

- `UNIQUE` для `login` и `email`
- `CHECK` на непустые строковые поля
- `CHECK` на базовую корректность email

### `folders`

Хранит папки пользователя.

Поля:

- `id` - первичный ключ
- `user_id` - владелец папки
- `name` - имя папки
- `created_at` - дата создания

Ограничения:

- внешний ключ на `users(id)`
- уникальность имени папки в рамках одного пользователя

### `messages`

Хранит письма.

Поля:

- `id` - первичный ключ
- `folder_id` - папка, в которой лежит сообщение
- `sender_id` - отправитель
- `recipient_email` - email получателя
- `subject` - тема письма
- `body` - текст письма
- `is_sent` - флаг отправки
- `created_at` - дата создания

Ограничения:

- внешний ключ на `folders(id)`
- внешний ключ на `users(id)`
- `CHECK` на непустые тему и тело
- `CHECK` на базовую корректность email получателя

## Индексы

В схеме используются следующие индексы:

- `users_login_key` и `users_email_key` создаются автоматически через `UNIQUE`
- `idx_users_full_name_lookup` для поиска пользователя по маске
- `idx_folders_owner_id` для выборки папок пользователя
- `idx_messages_folder_id` для выборки писем по папке
- `idx_messages_sender_id` для выборки писем по отправителю
- `idx_messages_folder_created_at` для выборки писем в папке с сортировкой по дате

## Запуск PostgreSQL

Для локального запуска используется `docker-compose.yaml`.

Поднять контейнер:

```bash
docker compose up --build -d
```

Проверить статус:

```bash
docker compose ps
```

Остановить контейнер:

```bash
docker compose down
```

## Проверка схемы и данных

Посмотреть таблицы:

```bash
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "\dt"
```

Проверить количество записей:

```bash
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from users;"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from folders;"
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select count(*) from messages;"
```

Выполнить произвольный запрос:

```bash
docker compose exec postgres psql -U mail_user -d mail_lab_db -c "select u.login, f.name, m.subject from users u join folders f on f.user_id = u.id join messages m on m.folder_id = f.id order by u.id, f.id, m.id limit 10;"
```

## Содержимое SQL-артефактов

- `schema.sql` создает таблицы `users`, `folders`, `messages`
- `data.sql` заполняет таблицы тестовыми данными
- `queries.sql` содержит запросы на создание пользователя, поиск пользователя, создание папки, создание и чтение писем
- `optimization.md` объясняет, какие индексы добавлены и как они влияют на планы выполнения
