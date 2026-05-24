# Оптимизация запросов

## Что оптимизировалось

Основные запросы в сервисе:

1. поиск пользователя по логину
2. поиск пользователя по маске имени
3. получение папок пользователя
4. получение писем в папке
5. получение письма по идентификатору

## Индексы и зачем они нужны

- `users(login)` и `users(email)` создаются автоматически за счет `UNIQUE`
- `idx_users_full_name_lookup` ускоряет поиск по `mask`
- `idx_folders_owner_id` ускоряет выборку папок по `user_id`
- `idx_messages_folder_id` ускоряет фильтрацию писем по папке
- `idx_messages_sender_id` ускоряет связи с отправителем
- `idx_messages_folder_created_at` покрывает частый запрос `WHERE folder_id = ? ORDER BY created_at DESC`

## EXPLAIN до и после

### Поиск пользователя по логину

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

### Получение папок пользователя

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

### Получение писем в папке

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

## Вывод

Для этой модели самый важный индекс — `idx_messages_folder_created_at`, потому что запрос списка писем выполняется чаще других и одновременно фильтрует по папке и сортирует по дате. Остальные индексы закрывают поиск пользователя и выборки по внешним ключам.
