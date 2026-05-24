#include "entities.hpp"

#include <stdexcept>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/datetime.hpp>

namespace mail_lab::domain {

namespace {

std::string ReadRequiredString(const userver::formats::json::Value& json,
                               const std::string& field) {
    if (!json.HasMember(field)) {
        throw std::runtime_error("Missing field: " + field);
    }
    return json[field].As<std::string>();
}

}  // namespace

userver::formats::json::Value UserRecord::ToPublicJson() const {
    userver::formats::json::ValueBuilder builder;
    if (id) builder["id"] = *id;
    builder["login"] = login;
    builder["email"] = email;
    builder["first_name"] = first_name;
    builder["last_name"] = last_name;
    builder["created_at"] = userver::utils::datetime::Timestring(created_at);
    return builder.ExtractValue();
}

userver::formats::json::Value FolderRecord::ToJson() const {
    userver::formats::json::ValueBuilder builder;
    if (id) builder["id"] = *id;
    builder["name"] = title;
    builder["user_id"] = owner_id;
    builder["created_at"] = userver::utils::datetime::Timestring(created_at);
    return builder.ExtractValue();
}

userver::formats::json::Value MessageRecord::ToJson() const {
    userver::formats::json::ValueBuilder builder;
    if (id) builder["id"] = *id;
    builder["folder_id"] = folder_id;
    builder["recipient_email"] = recipient_email;
    builder["subject"] = subject;
    builder["body"] = body;
    builder["created_at"] = userver::utils::datetime::Timestring(created_at);
    return builder.ExtractValue();
}

userver::formats::json::Value SessionToken::ToJson() const {
    userver::formats::json::ValueBuilder builder;
    builder["token"] = token;
    builder["user_id"] = user_id;
    builder["expires_at"] = userver::utils::datetime::Timestring(expires_at);
    return builder.ExtractValue();
}

UserDraft ParseUserDraft(const userver::formats::json::Value& json) {
    return {
        .login = ReadRequiredString(json, "login"),
        .email = ReadRequiredString(json, "email"),
        .first_name = ReadRequiredString(json, "first_name"),
        .last_name = ReadRequiredString(json, "last_name"),
        .password = ReadRequiredString(json, "password"),
    };
}

LoginDraft ParseLoginDraft(const userver::formats::json::Value& json) {
    return {
        .login = ReadRequiredString(json, "login"),
        .password = ReadRequiredString(json, "password"),
    };
}

FolderDraft ParseFolderDraft(const userver::formats::json::Value& json) {
    return {.name = ReadRequiredString(json, "name")};
}

MessageDraft ParseMessageDraft(const userver::formats::json::Value& json) {
    return {
        .recipient_email = ReadRequiredString(json, "recipient_email"),
        .subject = ReadRequiredString(json, "subject"),
        .body = ReadRequiredString(json, "body"),
    };
}

}  // namespace mail_lab::domain
