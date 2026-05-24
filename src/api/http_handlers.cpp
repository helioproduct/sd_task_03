#include "http_handlers.hpp"

#include <fstream>
#include <optional>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/datetime.hpp>

#include "domain/entities.hpp"
#include "domain/jwt_tools.hpp"

namespace mail_lab::api {

namespace {

std::shared_ptr<domain::MailRepository> SharedStorage(userver::storages::postgres::ClusterPtr pg_cluster) {
    static auto storage = std::make_shared<domain::MailRepository>(std::move(pg_cluster));
    return storage;
}

std::string ErrorJson(const std::string& message) {
    return userver::formats::json::ToString(userver::formats::json::MakeObject("error", message));
}

std::optional<int64_t> RequireAuthorizedUser(const userver::server::http::HttpRequest& request,
                                             std::string& response_body) {
    const auto auth_header = request.GetHeader(userver::http::headers::kAuthorization);
    if (auth_header.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        response_body = ErrorJson("Authorization header required");
        return std::nullopt;
    }

    constexpr std::string_view kBearerPrefix = "Bearer ";
    if (!auth_header.starts_with(kBearerPrefix)) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        response_body = ErrorJson("Invalid authorization header");
        return std::nullopt;
    }

    const auto token = std::string(auth_header.substr(kBearerPrefix.size()));
    const auto user_id = domain::JwtTools::ParseUserId(token);
    if (!user_id) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        response_body = ErrorJson("Invalid token");
        return std::nullopt;
    }

    return user_id;
}

bool IsEmpty(const std::string& value) {
    return value.empty();
}

bool IsEmailValid(const std::string& value) {
    return value.find('@') != std::string::npos;
}

int64_t ParseIdOrReject(const userver::server::http::HttpRequest& request,
                        const std::string& raw,
                        const std::string&,
                        bool& ok) {
    try {
        ok = true;
        return std::stoll(raw);
    } catch (...) {
        ok = false;
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return 0;
    }
}

std::string LoadFile(const std::string& path) {
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

}  // namespace

JsonApiHandlerBase::JsonApiHandlerBase(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(SharedStorage(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster())) {}

std::string SignupHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                              userver::server::request::RequestContext&) const {
    try {
        const auto body = userver::formats::json::FromString(request.RequestBody());
        if (!body.HasMember("login") || !body.HasMember("email") || !body.HasMember("first_name") ||
            !body.HasMember("last_name") || !body.HasMember("password")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Missing required fields");
        }
        const auto draft = domain::ParseUserDraft(body);

        if (IsEmpty(draft.login)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Login is required");
        }
        if (IsEmpty(draft.email)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Email is required");
        }
        if (!IsEmailValid(draft.email)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Invalid email format");
        }
        if (IsEmpty(draft.first_name)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("First name is required");
        }
        if (IsEmpty(draft.last_name)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Last name is required");
        }
        if (IsEmpty(draft.password)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Password is required");
        }

        if (storage_->FindUserByLogin(draft.login)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
            return ErrorJson("Login is already taken");
        }
        if (storage_->EmailExists(draft.email)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
            return ErrorJson("Email is already taken");
        }

        const auto created = storage_->CreateUser(draft);
        if (!created) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
            return ErrorJson("Failed to create user");
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(created->ToPublicJson());
    } catch (...) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Invalid request body");
    }
}

std::string LoginHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                             userver::server::request::RequestContext&) const {
    try {
        const auto body = userver::formats::json::FromString(request.RequestBody());
        if (!body.HasMember("login")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Login is required");
        }
        if (!body.HasMember("password")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Password is required");
        }
        const auto draft = domain::ParseLoginDraft(body);

        if (IsEmpty(draft.login)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Login is required");
        }
        if (IsEmpty(draft.password)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Password is required");
        }

        const auto user = storage_->CheckCredentials(draft.login, draft.password);
        if (!user || !user->id) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
            return ErrorJson("Invalid credentials");
        }

        domain::SessionToken session{
            .token = domain::JwtTools::BuildToken(*user->id),
            .user_id = *user->id,
            .expires_at = std::chrono::system_clock::now() + std::chrono::hours(24),
        };
        return userver::formats::json::ToString(session.ToJson());
    } catch (...) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Invalid request body");
    }
}

std::string UserLookupHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                  userver::server::request::RequestContext&) const {
    std::string auth_error;
    if (!RequireAuthorizedUser(request, auth_error)) return auth_error;

    const auto login = request.GetArg("login");
    if (login.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Login is required");
    }

    const auto user = storage_->FindUserByLogin(login);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("User not found");
    }

    return userver::formats::json::ToString(user->ToPublicJson());
}

std::string UserSearchHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                  userver::server::request::RequestContext&) const {
    std::string auth_error;
    if (!RequireAuthorizedUser(request, auth_error)) return auth_error;

    const auto mask = request.GetArg("mask");
    if (mask.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Mask is required");
    }

    auto users = storage_->SearchUsers(mask);
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& user : users) {
        builder.PushBack(user.ToPublicJson());
    }
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string FolderCreateHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                    userver::server::request::RequestContext&) const {
    std::string auth_error;
    const auto user_id = RequireAuthorizedUser(request, auth_error);
    if (!user_id) return auth_error;

    try {
        const auto body = userver::formats::json::FromString(request.RequestBody());
        if (!body.HasMember("name")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Name is required");
        }
        const auto draft = domain::ParseFolderDraft(body);
        if (IsEmpty(draft.name)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Name is required");
        }

        const auto folder = storage_->CreateFolder(*user_id, draft.name);
        if (!folder) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
            return ErrorJson("Folder already exists or invalid data");
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(folder->ToJson());
    } catch (...) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Invalid request body");
    }
}

std::string FolderListHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                  userver::server::request::RequestContext&) const {
    std::string auth_error;
    const auto user_id = RequireAuthorizedUser(request, auth_error);
    if (!user_id) return auth_error;

    const auto folders = storage_->ListFolders(*user_id);
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& folder : folders) {
        builder.PushBack(folder.ToJson());
    }
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string MessageCreateHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                     userver::server::request::RequestContext&) const {
    std::string auth_error;
    const auto user_id = RequireAuthorizedUser(request, auth_error);
    if (!user_id) return auth_error;

    bool ok = false;
    const auto folder_id = ParseIdOrReject(request, request.GetPathArg("folderId"), "Invalid folder ID", ok);
    if (!ok) return ErrorJson("Invalid folder ID");

    const auto folder = storage_->FindFolderById(folder_id);
    if (!folder) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("Folder not found");
    }
    if (folder->owner_id != *user_id) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("Access denied to this folder");
    }

    try {
        const auto body = userver::formats::json::FromString(request.RequestBody());
        if (!body.HasMember("subject")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Subject is required");
        }
        if (!body.HasMember("body")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Body is required");
        }
        if (!body.HasMember("recipient_email")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Recipient is required");
        }
        const auto draft = domain::ParseMessageDraft(body);

        if (IsEmpty(draft.subject)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Subject is required");
        }
        if (IsEmpty(draft.body)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Body is required");
        }
        if (IsEmpty(draft.recipient_email)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Recipient is required");
        }
        if (!IsEmailValid(draft.recipient_email)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return ErrorJson("Invalid email format");
        }

        const auto message = storage_->CreateMessage(folder_id, *user_id, draft);
        if (!message) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return ErrorJson("Failed to create message");
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(message->ToJson());
    } catch (...) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson("Invalid request body");
    }
}

std::string MessageListHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                   userver::server::request::RequestContext&) const {
    std::string auth_error;
    const auto user_id = RequireAuthorizedUser(request, auth_error);
    if (!user_id) return auth_error;

    bool ok = false;
    const auto folder_id = ParseIdOrReject(request, request.GetPathArg("folderId"), "Invalid folder ID", ok);
    if (!ok) return ErrorJson("Invalid folder ID");

    const auto folder = storage_->FindFolderById(folder_id);
    if (!folder) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("Folder not found");
    }
    if (folder->owner_id != *user_id) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("Access denied to this folder");
    }

    auto messages = storage_->ListMessages(folder_id);
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& message : messages) {
        builder.PushBack(message.ToJson());
    }
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string MessageReadHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                   userver::server::request::RequestContext&) const {
    std::string auth_error;
    const auto user_id = RequireAuthorizedUser(request, auth_error);
    if (!user_id) return auth_error;

    bool ok = false;
    const auto message_id = ParseIdOrReject(request, request.GetPathArg("messageId"), "Invalid message ID", ok);
    if (!ok) return ErrorJson("Invalid message ID");

    const auto message = storage_->FindMessageById(message_id);
    if (!message) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return ErrorJson("Message not found");
    }

    const auto folder = storage_->FindFolderById(message->folder_id);
    if (!folder || folder->owner_id != *user_id) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return ErrorJson("Access denied to this message");
    }

    return userver::formats::json::ToString(message->ToJson());
}

std::string SwaggerPageHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                   userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType("text/html; charset=utf-8");
    return R"(<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>Mail API Swagger</title>
    <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css">
  </head>
  <body>
    <div id="swagger-ui"></div>
    <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js"></script>
    <script>
      window.ui = SwaggerUIBundle({url: '/openapi.yaml', dom_id: '#swagger-ui'});
    </script>
  </body>
</html>)";
}

std::string OpenApiHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType("application/yaml");
    return LoadFile("/app/openapi.yaml");
}

std::string PingTextHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                                userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType("text/plain; charset=utf-8");
    return "pong\n";
}

void AppendHandlers(userver::components::ComponentList& component_list) {
    component_list.Append<SignupHandler>();
    component_list.Append<LoginHandler>();
    component_list.Append<UserLookupHandler>();
    component_list.Append<UserSearchHandler>();
    component_list.Append<FolderCreateHandler>();
    component_list.Append<FolderListHandler>();
    component_list.Append<MessageCreateHandler>();
    component_list.Append<MessageListHandler>();
    component_list.Append<MessageReadHandler>();
    component_list.Append<SwaggerPageHandler>();
    component_list.Append<OpenApiHandler>();
    component_list.Append<PingTextHandler>();
}

}  // namespace mail_lab::api
