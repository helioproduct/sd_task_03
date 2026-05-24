#pragma once

#include <memory>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "domain/mail_repository.hpp"

namespace mail_lab::api {

class JsonApiHandlerBase : public userver::server::handlers::HttpHandlerBase {
public:
    JsonApiHandlerBase(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

protected:
    std::shared_ptr<domain::MailRepository> storage_;
};

class SignupHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-signup";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class LoginHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-login";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class UserLookupHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-lookup";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class UserSearchHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-search";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class FolderCreateHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-folder-create";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class FolderListHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-folder-list";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class MessageCreateHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-message-create";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class MessageListHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-message-list";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class MessageReadHandler final : public JsonApiHandlerBase {
public:
    static constexpr std::string_view kName = "handler-message-read";
    using JsonApiHandlerBase::JsonApiHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class SwaggerPageHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-swagger-page";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class OpenApiHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-openapi";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

class PingTextHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-ping-text";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                   userver::server::request::RequestContext&) const override;
};

void AppendHandlers(userver::components::ComponentList& component_list);

}  // namespace mail_lab::api
