DROP TABLE IF EXISTS messages CASCADE;
DROP TABLE IF EXISTS folders CASCADE;
DROP TABLE IF EXISTS users CASCADE;

CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(255) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    first_name VARCHAR(255) NOT NULL,
    last_name VARCHAR(255) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_users_login_not_blank CHECK (length(trim(login)) > 0),
    CONSTRAINT chk_users_email_not_blank CHECK (length(trim(email)) > 0),
    CONSTRAINT chk_users_email_format CHECK (email LIKE '%@%'),
    CONSTRAINT chk_users_first_name_not_blank CHECK (length(trim(first_name)) > 0),
    CONSTRAINT chk_users_last_name_not_blank CHECK (length(trim(last_name)) > 0)
);

CREATE TABLE folders (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_folders_name_not_blank CHECK (length(trim(name)) > 0),
    CONSTRAINT uq_folders_user_name UNIQUE (user_id, name)
);

CREATE TABLE messages (
    id BIGSERIAL PRIMARY KEY,
    folder_id BIGINT NOT NULL REFERENCES folders(id) ON DELETE CASCADE,
    sender_id BIGINT NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    recipient_email VARCHAR(255) NOT NULL,
    subject VARCHAR(500) NOT NULL,
    body TEXT NOT NULL,
    is_sent BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_messages_recipient_email_format CHECK (recipient_email LIKE '%@%'),
    CONSTRAINT chk_messages_subject_not_blank CHECK (length(trim(subject)) > 0),
    CONSTRAINT chk_messages_body_not_blank CHECK (length(trim(body)) > 0)
);

CREATE INDEX idx_users_full_name_lookup
    ON users (lower(first_name || ' ' || last_name) text_pattern_ops);

CREATE INDEX idx_folders_owner_id
    ON folders (user_id);

CREATE INDEX idx_messages_folder_id
    ON messages (folder_id);

CREATE INDEX idx_messages_sender_id
    ON messages (sender_id);

CREATE INDEX idx_messages_folder_created_at
    ON messages (folder_id, created_at DESC);
