#include "mail_repository.hpp"

#include <userver/crypto/hash.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

namespace mail_lab::domain {

MailRepository::MailRepository(userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

std::string MailRepository::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

std::optional<UserRecord> MailRepository::CreateUser(const UserDraft& draft) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO users (login, email, first_name, last_name, password_hash) "
            "VALUES ($1, $2, $3, $4, $5) "
            "RETURNING id, login, email, first_name, last_name, password_hash, created_at",
            draft.login,
            draft.email,
            draft.first_name,
            draft.last_name,
            HashPassword(draft.password));

        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<UserRecord>(userver::storages::postgres::kRowTag);
    } catch (const userver::storages::postgres::UniqueViolation&) {
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<UserRecord> MailRepository::FindUserByLogin(const std::string& login) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users WHERE login = $1",
            login);
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<UserRecord>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<UserRecord> MailRepository::FindUserById(int64_t user_id) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users WHERE id = $1",
            user_id);
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<UserRecord>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<UserRecord> MailRepository::SearchUsers(const std::string& mask) {
    try {
        const auto like_mask = "%" + mask + "%";
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users "
            "WHERE lower(first_name || ' ' || last_name) LIKE lower($1) "
            "   OR lower(first_name) LIKE lower($1) "
            "   OR lower(last_name) LIKE lower($1) "
            "ORDER BY id",
            like_mask);
        return result.AsContainer<std::vector<UserRecord>>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return {};
    }
}

std::optional<UserRecord> MailRepository::CheckCredentials(const std::string& login,
                                                           const std::string& password) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users WHERE login = $1 AND password_hash = $2",
            login,
            HashPassword(password));
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<UserRecord>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return std::nullopt;
    }
}

bool MailRepository::EmailExists(const std::string& email) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT 1 FROM users WHERE email = $1",
            email);
        return !result.IsEmpty();
    } catch (...) {
        return false;
    }
}

std::optional<FolderRecord> MailRepository::CreateFolder(int64_t user_id, const std::string& folder_name) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO folders (user_id, name) VALUES ($1, $2) "
            "RETURNING id, user_id, name, created_at",
            user_id,
            folder_name);
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<FolderRecord>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<FolderRecord> MailRepository::ListFolders(int64_t user_id) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, name, created_at FROM folders WHERE user_id = $1 ORDER BY id",
            user_id);
        return result.AsContainer<std::vector<FolderRecord>>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return {};
    }
}

std::optional<FolderRecord> MailRepository::FindFolderById(int64_t folder_id) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, name, created_at FROM folders WHERE id = $1",
            folder_id);
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<FolderRecord>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<MessageRecord> MailRepository::CreateMessage(int64_t folder_id,
                                                           int64_t user_id,
                                                           const MessageDraft& draft) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent) "
            "VALUES ($1, $2, $3, $4, $5, $6) "
            "RETURNING id, folder_id, sender_id, recipient_email, subject, body, created_at",
            folder_id,
            user_id,
            draft.recipient_email,
            draft.subject,
            draft.body,
            false);
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<MessageRecord>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<MessageRecord> MailRepository::ListMessages(int64_t folder_id) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, folder_id, sender_id, recipient_email, subject, body, created_at "
            "FROM messages WHERE folder_id = $1 ORDER BY created_at DESC",
            folder_id);
        return result.AsContainer<std::vector<MessageRecord>>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return {};
    }
}

std::optional<MessageRecord> MailRepository::FindMessageById(int64_t message_id) {
    try {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, folder_id, sender_id, recipient_email, subject, body, created_at "
            "FROM messages WHERE id = $1",
            message_id);
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        return result.AsSingleRow<MessageRecord>(userver::storages::postgres::kRowTag);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace mail_lab::domain
