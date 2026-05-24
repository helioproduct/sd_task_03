#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json/value.hpp>

namespace mail_lab::domain {

struct UserRecord {
    std::optional<int64_t> id;
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password_hash;
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToPublicJson() const;
};

struct FolderRecord {
    std::optional<int64_t> id;
    int64_t owner_id{0};
    std::string title;
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToJson() const;
};

struct MessageRecord {
    std::optional<int64_t> id;
    int64_t folder_id{0};
    int64_t author_id{0};
    std::string recipient_email;
    std::string subject;
    std::string body;
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToJson() const;
};

struct SessionToken {
    std::string token;
    int64_t user_id{0};
    std::chrono::system_clock::time_point expires_at;

    userver::formats::json::Value ToJson() const;
};

struct UserDraft {
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password;
};

struct LoginDraft {
    std::string login;
    std::string password;
};

struct FolderDraft {
    std::string name;
};

struct MessageDraft {
    std::string recipient_email;
    std::string subject;
    std::string body;
};

UserDraft ParseUserDraft(const userver::formats::json::Value& json);
LoginDraft ParseLoginDraft(const userver::formats::json::Value& json);
FolderDraft ParseFolderDraft(const userver::formats::json::Value& json);
MessageDraft ParseMessageDraft(const userver::formats::json::Value& json);

}  // namespace mail_lab::domain
