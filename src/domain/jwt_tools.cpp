#include "jwt_tools.hpp"

#include <chrono>
#include <string>

#include <jwt-cpp/jwt.h>

namespace mail_lab::domain {

namespace {

constexpr const char* kSecret = "your-secret-key-change-in-production";

}  // namespace

std::string JwtTools::BuildToken(int64_t user_id) {
    return ::jwt::create<::jwt::traits::kazuho_picojson>()
        .set_type("JWT")
        .set_payload_claim("user_id", ::jwt::claim(std::to_string(user_id)))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
        .sign(::jwt::algorithm::hs256{kSecret});
}

std::optional<int64_t> JwtTools::ParseUserId(const std::string& token) {
    try {
        auto decoded = ::jwt::decode<::jwt::traits::kazuho_picojson>(token);
        auto verifier = ::jwt::verify<::jwt::traits::kazuho_picojson>()
            .allow_algorithm(::jwt::algorithm::hs256{kSecret});
        verifier.verify(decoded);

        if (!decoded.has_payload_claim("user_id")) {
            return std::nullopt;
        }

        const auto claim = decoded.get_payload_claim("user_id");
        return std::stoll(claim.as_string());
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace mail_lab::domain
