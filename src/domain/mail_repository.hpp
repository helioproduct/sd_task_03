#pragma once

#include <optional>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "entities.hpp"

namespace mail_lab::domain {

class MailRepository {
public:
    explicit MailRepository(const std::string& db_path);
    ~MailRepository();

    std::optional<UserRecord> CreateUser(const UserDraft& draft);
    std::optional<UserRecord> FindUserByLogin(const std::string& login);
    std::optional<UserRecord> FindUserById(int64_t user_id);
    std::vector<UserRecord> SearchUsers(const std::string& mask);
    std::optional<UserRecord> CheckCredentials(const std::string& login, const std::string& password);
    bool EmailExists(const std::string& email);

    std::optional<FolderRecord> CreateFolder(int64_t user_id, const std::string& folder_name);
    std::vector<FolderRecord> ListFolders(int64_t user_id);
    std::optional<FolderRecord> FindFolderById(int64_t folder_id);

    std::optional<MessageRecord> CreateMessage(int64_t folder_id, int64_t user_id, const MessageDraft& draft);
    std::vector<MessageRecord> ListMessages(int64_t folder_id);
    std::optional<MessageRecord> FindMessageById(int64_t message_id);

private:
    sqlite3* db_{nullptr};

    void InitSchema();
    bool RunSql(const std::string& sql);
    std::string HashPassword(const std::string& password) const;
    std::optional<int64_t> LastInsertId() const;
};

}  // namespace mail_lab::domain
