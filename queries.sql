-- 1. Create user
INSERT INTO users (login, email, first_name, last_name, password_hash)
VALUES ($1, $2, $3, $4, $5)
RETURNING id, login, email, first_name, last_name, created_at;

-- 2. Find user by login
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE login = $1;

-- 3. Search users by name mask
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE lower(first_name || ' ' || last_name) LIKE lower($1)
   OR lower(first_name) LIKE lower($1)
   OR lower(last_name) LIKE lower($1)
ORDER BY id;

-- 4. Create folder
INSERT INTO folders (user_id, name)
VALUES ($1, $2)
RETURNING id, user_id, name, created_at;

-- 5. List folders for user
SELECT id, user_id, name, created_at
FROM folders
WHERE user_id = $1
ORDER BY id;

-- 6. Create message in folder
INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent)
VALUES ($1, $2, $3, $4, $5, FALSE)
RETURNING id, folder_id, sender_id, recipient_email, subject, body, created_at;

-- 7. List messages in folder
SELECT id, folder_id, sender_id, recipient_email, subject, body, created_at
FROM messages
WHERE folder_id = $1
ORDER BY created_at DESC;

-- 8. Get message by id
SELECT id, folder_id, sender_id, recipient_email, subject, body, created_at
FROM messages
WHERE id = $1;

-- Auxiliary: authenticate user
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE login = $1 AND password_hash = $2;

-- Auxiliary: verify folder ownership
SELECT id
FROM folders
WHERE id = $1 AND user_id = $2;

-- Auxiliary: verify message ownership through folder
SELECT m.id
FROM messages m
JOIN folders f ON f.id = m.folder_id
WHERE m.id = $1 AND f.user_id = $2;
