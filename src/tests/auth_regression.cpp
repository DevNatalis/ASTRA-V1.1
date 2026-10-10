#define SVCHOST_AUTH_TESTS
#include <Auth/auth_manager.hpp>
#include <iostream>
#include <stdexcept>

static void Check(bool condition, const char* name)
{
    if (!condition) throw std::runtime_error(name);
    std::cout << "PASS: " << name << '\n';
}

struct AuthRegression
{
    static void Run()
    {
        AuthManager auth;
        Check(auth.PinLooksConfigured(), "configured pin passes prefix validation");
        auth.sessionid_ = "test-session+&=";
        Check(auth.SessionId() == auth.sessionid_, "pre-login session is available");
        const auto login = auth.BuildLoginFields("test user", "p&=+ word");
        const auto registration = auth.BuildRegisterFields("test user", "p&=+ word", "test-key");
        Check(login.find("&sessionid=test-session%2B%26%3D&") != std::string::npos,
            "login sends the initialized session, URL encoded");
        Check(registration.find("&sessionid=test-session%2B%26%3D&") != std::string::npos,
            "registration sends the initialized session, URL encoded");
        Check(login.find("&pass=p%26%3D%2B%20word&") != std::string::npos,
            "password special characters are preserved");
        Check(!auth.IsSessionValid(), "an API session alone never authorizes access");
        auth.session_.armed = true;
        auth.session_.maskedSid = auth.MaskSidLocked("authenticated-session");
        Check(auth.SessionId() == "authenticated-session", "authenticated requests use the protected session");
        Check(auth.ClassifyMessage("Invalid Session") == AuthResult::SessionExpired,
            "session errors are classified case insensitively");
        Check(auth.ClassifyMessage("Invalid License Key") == AuthResult::InvalidLicense,
            "license errors are not reported as password errors");
        Check(auth.ClassifyMessage("Application disabled") == AuthResult::UnknownError,
            "unknown server errors are not reported as password errors");
        Check(auth.ServerMessage("Invalid Session\r\nRetry") == "Invalid Session  Retry",
            "server error preserves its cause and removes control characters");
        AuthPolicy::License license;
        auto reply = nlohmann::json::parse(R"({"success":true,"info":{"username":"test","subscriptions":[{"subscription":"default","expiry":"2000"}]}})");
        Check(AuthPolicy::ParseLicense(reply, 1000, license), "active subscription is accepted");
        Check(!AuthPolicy::ParseLicense(reply, 2000, license), "expired subscription is rejected");
        reply["success"] = false;
        Check(!AuthPolicy::ParseLicense(reply, 1000, license), "failed authentication cannot authorize access");
    }
};

int main()
{
    try { AuthRegression::Run(); }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
