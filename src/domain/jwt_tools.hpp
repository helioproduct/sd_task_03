#pragma once

#include <optional>
#include <string>

namespace mail_lab::domain {

class JwtTools {
public:
    static std::string BuildToken(int64_t user_id);
    static std::optional<int64_t> ParseUserId(const std::string& token);
};

}  // namespace mail_lab::domain
