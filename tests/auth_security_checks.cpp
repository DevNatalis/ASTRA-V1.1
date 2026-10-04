#include <Auth/auth_manager.hpp>
#include <iostream>

int main() {
    int failures = 0;
    auto check = [&](bool ok, const char* name) {
        if (!ok) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
    };
    check(AuthPolicy::FormEncode("a&b+c=1%?#") == "a%26b%2Bc%3D1%25%3F%23", "form separators");
    check(AuthPolicy::FormEncode("\xC3\xA7") == "%C3%A7", "UTF-8 bytes");
    using nlohmann::json;
    const auto valid = json::parse(R"({"success":true,"info":{"username":"test","subscriptions":[{"subscription":"standard","expiry":"2000"}]}})");
    AuthPolicy::License license;
    check(AuthPolicy::ParseLicense(valid, 1000, license), "valid license");
    check(license.user == "test" && license.expiry == "2000", "license fields");
    for (const char* expiry : {"0", "-1", "1000", "999", "2000junk", "", "99999999999999999999999"}) {
        auto j = valid;
        j["info"]["subscriptions"][0]["expiry"] = expiry;
        check(!AuthPolicy::ParseLicense(j, 1000, license), "reject invalid/expired expiry");
        check(license.user.empty(), "clear previous authorization");
    }
    for (const char* raw : {R"({"success":true})", R"({"success":false})", R"({"success":"true"})", R"({"success":true,"info":{"username":"test","subscriptions":[]}})"})
        check(!AuthPolicy::ParseLicense(json::parse(raw), 1000, license), "reject incomplete response");
    auto multi = valid;
    multi["info"]["subscriptions"][0]["expiry"] = "999";
    multi["info"]["subscriptions"].push_back(valid["info"]["subscriptions"][0]);
    check(AuthPolicy::ParseLicense(multi, 1000, license), "valid subscription after expired one");
    auto missing = valid;
    missing["info"]["subscriptions"][0].erase("expiry");
    check(!AuthPolicy::ParseLicense(missing, 1000, license), "missing expiry");
    check(!AuthPolicy::ParseLicense(valid, -1, license), "invalid clock");
    AuthManager manager;
    manager.async.ready = true;
    manager.async.result = AuthResult::Ok;
    manager.requestInProgress = true;
    manager.ConsumeResult();
    check(!manager.IsAuthenticated(), "unfinished worker cannot grant access");
    std::cout << "Failures: " << failures << '\n';
    return failures ? 1 : 0;
}
