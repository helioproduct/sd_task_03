#include "mail_repository.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <userver/crypto/hash.hpp>

namespace mail_lab::domain {

namespace {

std::chrono::system_clock::time_point ParseDbTime(const char* value) {
    if (!value) return std::chrono::system_clock::now();

    std::tm tm = {};
    std::istringstream stream(value);
    stream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (stream.fail()) return std::chrono::system_clock::now();

    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

}  // namespace

MailRepository::MailRepository(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Cannot open SQLite database");
    }
    sqlite3_busy_timeout(db_, 5000);
    RunSql("PRAGMA foreign_keys = ON;");
    RunSql("PRAGMA journal_mode = WAL;");
    InitSchema();
}

MailRepository::~MailRepository() {
    if (db_) sqlite3_close(db_);
}

void MailRepository::InitSchema() {
    RunSql(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            login TEXT NOT NULL UNIQUE,
            email TEXT NOT NULL UNIQUE,
            first_name TEXT NOT NULL,
            last_name TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
            UNIQUE (user_id, name)
        );

        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            folder_id INTEGER NOT NULL,
            sender_user_id INTEGER NOT NULL,
            recipient_email TEXT NOT NULL,
            subject TEXT NOT NULL,
            body TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE,
            FOREIGN KEY (sender_user_id) REFERENCES users(id) ON DELETE CASCADE
        );
    )");
}

bool MailRepository::RunSql(const std::string& sql) {
    char* error_message = nullptr;
    const auto rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_message);
    if (rc == SQLITE_OK) return true;
    if (error_message) sqlite3_free(error_message);
    return false;
}

std::string MailRepository::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

std::optional<int64_t> MailRepository::LastInsertId() const {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT last_insert_rowid()", -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    std::optional<int64_t> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<UserRecord> MailRepository::CreateUser(const UserDraft& draft) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO users (login, email, first_name, last_name, password_hash) VALUES (?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    const auto password_hash = HashPassword(draft.password);
    sqlite3_bind_text(stmt, 1, draft.login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, draft.email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, draft.first_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, draft.last_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, password_hash.c_str(), -1, SQLITE_STATIC);

    const auto rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return std::nullopt;

    const auto id = LastInsertId();
    return id ? FindUserById(*id) : std::nullopt;
}

std::optional<UserRecord> MailRepository::FindUserByLogin(const std::string& login) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, login, email, first_name, last_name, password_hash, created_at FROM users WHERE login = ?";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    std::optional<UserRecord> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = UserRecord{
            .id = sqlite3_column_int64(stmt, 0),
            .login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            .email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            .first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            .last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            .password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)),
            .created_at = ParseDbTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))),
        };
    }

    sqlite3_finalize(stmt);
    return user;
}

std::optional<UserRecord> MailRepository::FindUserById(int64_t user_id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, login, email, first_name, last_name, password_hash, created_at FROM users WHERE id = ?";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, user_id);

    std::optional<UserRecord> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = UserRecord{
            .id = sqlite3_column_int64(stmt, 0),
            .login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            .email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            .first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            .last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            .password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)),
            .created_at = ParseDbTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))),
        };
    }

    sqlite3_finalize(stmt);
    return user;
}

std::vector<UserRecord> MailRepository::SearchUsers(const std::string& mask) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, login, email, first_name, last_name, password_hash, created_at "
        "FROM users WHERE lower(first_name || ' ' || last_name) LIKE lower(?) "
        "OR lower(first_name) LIKE lower(?) OR lower(last_name) LIKE lower(?) ORDER BY id";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }

    const auto like_mask = "%" + mask + "%";
    sqlite3_bind_text(stmt, 1, like_mask.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, like_mask.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, like_mask.c_str(), -1, SQLITE_STATIC);

    std::vector<UserRecord> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        users.push_back(UserRecord{
            .id = sqlite3_column_int64(stmt, 0),
            .login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            .email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            .first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            .last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            .password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)),
            .created_at = ParseDbTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))),
        });
    }

    sqlite3_finalize(stmt);
    return users;
}

std::optional<UserRecord> MailRepository::CheckCredentials(const std::string& login,
                                                           const std::string& password) {
    const auto user = FindUserByLogin(login);
    if (!user) return std::nullopt;
    return user->password_hash == HashPassword(password) ? user : std::nullopt;
}

bool MailRepository::EmailExists(const std::string& email) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT 1 FROM users WHERE email = ?", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

std::optional<FolderRecord> MailRepository::CreateFolder(int64_t user_id, const std::string& folder_name) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "INSERT INTO folders (user_id, name) VALUES (?, ?)", -1, &stmt, nullptr) !=
        SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, folder_name.c_str(), -1, SQLITE_STATIC);
    const auto rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return std::nullopt;

    const auto id = LastInsertId();
    return id ? FindFolderById(*id) : std::nullopt;
}

std::vector<FolderRecord> MailRepository::ListFolders(int64_t user_id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT id, user_id, name, created_at FROM folders WHERE user_id = ? ORDER BY id",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }

    sqlite3_bind_int64(stmt, 1, user_id);
    std::vector<FolderRecord> folders;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        folders.push_back(FolderRecord{
            .id = sqlite3_column_int64(stmt, 0),
            .owner_id = sqlite3_column_int64(stmt, 1),
            .title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            .created_at = ParseDbTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))),
        });
    }

    sqlite3_finalize(stmt);
    return folders;
}

std::optional<FolderRecord> MailRepository::FindFolderById(int64_t folder_id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT id, user_id, name, created_at FROM folders WHERE id = ?", -1, &stmt,
                           nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, folder_id);
    std::optional<FolderRecord> folder;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        folder = FolderRecord{
            .id = sqlite3_column_int64(stmt, 0),
            .owner_id = sqlite3_column_int64(stmt, 1),
            .title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            .created_at = ParseDbTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))),
        };
    }

    sqlite3_finalize(stmt);
    return folder;
}

std::optional<MessageRecord> MailRepository::CreateMessage(int64_t folder_id,
                                                           int64_t user_id,
                                                           const MessageDraft& draft) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO messages (folder_id, sender_user_id, recipient_email, subject, body) VALUES (?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int64(stmt, 2, user_id);
    sqlite3_bind_text(stmt, 3, draft.recipient_email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, draft.subject.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, draft.body.c_str(), -1, SQLITE_STATIC);

    const auto rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return std::nullopt;

    const auto id = LastInsertId();
    return id ? FindMessageById(*id) : std::nullopt;
}

std::vector<MessageRecord> MailRepository::ListMessages(int64_t folder_id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, folder_id, sender_user_id, recipient_email, subject, body, created_at "
        "FROM messages WHERE folder_id = ? ORDER BY id";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }

    sqlite3_bind_int64(stmt, 1, folder_id);
    std::vector<MessageRecord> messages;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        messages.push_back(MessageRecord{
            .id = sqlite3_column_int64(stmt, 0),
            .folder_id = sqlite3_column_int64(stmt, 1),
            .author_id = sqlite3_column_int64(stmt, 2),
            .recipient_email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            .subject = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            .body = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)),
            .created_at = ParseDbTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))),
        });
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::optional<MessageRecord> MailRepository::FindMessageById(int64_t message_id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id, folder_id, sender_user_id, recipient_email, subject, body, created_at "
        "FROM messages WHERE id = ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, message_id);
    std::optional<MessageRecord> message;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        message = MessageRecord{
            .id = sqlite3_column_int64(stmt, 0),
            .folder_id = sqlite3_column_int64(stmt, 1),
            .author_id = sqlite3_column_int64(stmt, 2),
            .recipient_email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            .subject = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            .body = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)),
            .created_at = ParseDbTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))),
        };
    }

    sqlite3_finalize(stmt);
    return message;
}

}  // namespace mail_lab::domain
