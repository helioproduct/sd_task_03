# Оптимизация запросов

## Цель

Для варианта "Электронная почта" наиболее важны запросы:

1. поиск пользователя по логину;
2. поиск пользователя по имени или фамилии по маске;
3. получение списка папок пользователя;
4. получение списка писем в папке с сортировкой по дате;
5. получение письма по идентификатору.

## Использованные индексы

### 1. Поиск пользователя по логину

Запрос:

```sql
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE login = $1;
```

Оптимизация:

- поле `login` объявлено как `UNIQUE`;
- PostgreSQL автоматически создает индекс `users_login_key`.

Ожидаемый эффект:

- вместо полного просмотра таблицы используется `Index Scan`.

Пример:

```sql
EXPLAIN SELECT id, login FROM users WHERE login = 'alex.ivanov';
```

План:

```text
Index Scan using users_login_key on users
  Index Cond: ((login)::text = 'alex.ivanov'::text)
```

### 2. Поиск пользователя по маске

Запрос:

```sql
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE lower(first_name || ' ' || last_name) LIKE lower($1)
   OR lower(first_name) LIKE lower($1)
   OR lower(last_name) LIKE lower($1)
ORDER BY id;
```

Оптимизация:

```sql
CREATE INDEX idx_users_full_name_lookup
    ON users (lower(first_name || ' ' || last_name) text_pattern_ops);
```

Ожидаемый эффект:

- уменьшается стоимость поиска по маске полного имени;
- запрос лучше масштабируется при увеличении числа пользователей.

Пример:

```sql
EXPLAIN
SELECT id, login
FROM users
WHERE lower(first_name || ' ' || last_name) LIKE 'alex%';
```

План:

```text
Bitmap Heap Scan on users
  Recheck Cond: (lower(((first_name)::text || ' '::text) || (last_name)::text) ~~ 'alex%'::text)
  ->  Bitmap Index Scan on idx_users_full_name_lookup
```

### 3. Список папок пользователя

Запрос:

```sql
SELECT id, user_id, name, created_at
FROM folders
WHERE user_id = $1
ORDER BY id;
```

Оптимизация:

```sql
CREATE INDEX idx_folders_owner_id
    ON folders (user_id);
```

Ожидаемый эффект:

- выборка папок владельца выполняется по индексу вместо `Seq Scan`.

Пример:

```sql
EXPLAIN SELECT id, name FROM folders WHERE user_id = 1 ORDER BY id;
```

План:

```text
Index Scan using idx_folders_owner_id on folders
  Index Cond: (user_id = 1)
```

### 4. Список писем в папке

Запрос:

```sql
SELECT id, folder_id, sender_id, recipient_email, subject, body, created_at
FROM messages
WHERE folder_id = $1
ORDER BY created_at DESC;
```

Оптимизация:

```sql
CREATE INDEX idx_messages_folder_created_at
    ON messages (folder_id, created_at DESC);
```

Почему это важно:

- это один из самых частых запросов в почтовой системе;
- запрос одновременно фильтрует по `folder_id` и сортирует по `created_at DESC`.

Ожидаемый эффект:

- сервер читает строки в нужном порядке сразу из индекса;
- дополнительная сортировка не требуется.

Пример:

```sql
EXPLAIN
SELECT id, subject, created_at
FROM messages
WHERE folder_id = 1
ORDER BY created_at DESC;
```

План:

```text
Index Scan using idx_messages_folder_created_at on messages
  Index Cond: (folder_id = 1)
```

### 5. Получение письма по идентификатору

Запрос:

```sql
SELECT id, folder_id, sender_id, recipient_email, subject, body, created_at
FROM messages
WHERE id = $1;
```

Оптимизация:

- поле `id` является первичным ключом;
- PostgreSQL использует индекс первичного ключа автоматически.

Пример:

```sql
EXPLAIN SELECT id, subject FROM messages WHERE id = 1;
```

План:

```text
Index Scan using messages_pkey on messages
  Index Cond: (id = 1)
```

## Итог

Наиболее полезный индекс для этого варианта - `idx_messages_folder_created_at`, потому что он оптимизирует рабочий сценарий просмотра писем в папке и одновременно устраняет отдельную сортировку. Индексы по логину, владельцу папки и первичным ключам обеспечивают быстрый доступ к основным сущностям, а индекс `idx_users_full_name_lookup` улучшает поиск пользователей по маске.
