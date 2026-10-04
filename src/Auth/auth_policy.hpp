#pragma once
#include <string>
#include <cstdint>
#include <Security/Api/json.hpp>

namespace AuthPolicy {
inline std::string FormEncode(const std::string& value) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
            out += static_cast<char>(c);
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}

struct License {
    std::string user, subscription, expiry;
};

// Reject incomplete responses. A license must have an explicit future expiry.
inline bool ParseLicense(const nlohmann::json& response, std::int64_t now, License& result) {
    result = {};
    if (now <= 0) return false;
    try {
        if (!response.at("success").get<bool>()) return false;
        const auto& info = response.at("info");
        const auto user = info.at("username").get<std::string>();
        const auto& subscriptions = info.at("subscriptions");
        if (user.empty() || !subscriptions.is_array() || subscriptions.empty()) return false;
        for (const auto& sub : subscriptions) {
            try {
                const auto name = sub.at("subscription").get<std::string>();
                const auto expiry = sub.at("expiry").get<std::string>();
                if (name.empty() || expiry.empty() || expiry.find_first_not_of("0123456789") != std::string::npos)
                    continue;
                size_t consumed = 0;
                const auto timestamp = std::stoll(expiry, &consumed);
                if (consumed != expiry.size() || timestamp <= now) continue;
                result = { user, name, expiry };
                return true;
            } catch (...) { /* An invalid entry never grants access. */ }
        }
    } catch (...) {}
    return false;
}
}
