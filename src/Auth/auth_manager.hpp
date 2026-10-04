#pragma once
// language: C++, file: Auth/auth_manager.hpp, target: Windows x64, MSVC
// KeyAuth API 1.3 — AuthManager centralizado para ASTRA.
// Credenciais: name=Natalismonteiro2's Application ownerid=LwlTMUzKJV version=1.0
// NÃO inclui Application Secret (API 1.3 nao exige no cliente).
// NÃO inclui Seller Key, Admin Key ou qualquer chave administrativa.

#include <windows.h>
#include <sddl.h>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <chrono>
#include <ctime>
#include <Auth/auth_policy.hpp>

#include <Security/xorstr.hpp>
#include <Security/Api/json.hpp>
#include <Security/Api/curl/curl.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "libcurl.lib")

// ─── Resultados de autenticação ──────────────────────────────────────────────
enum class AuthResult
{
    Ok,
    InvalidCredentials,
    HwidMismatch,
    InvalidLicense,
    ExpiredLicense,
    LicenseAlreadyUsed,
    UserExists,
    NoInternet,
    RateLimit,
    SubscriptionExpired,
    NotInitialized,
    InProgress,
    Locked,
    UnknownError
};

// ─── Página de autenticação ───────────────────────────────────────────────────
enum class AuthPage
{
    Login,
    Register,
    Authenticated
};

// ─── Resultado assíncrono ─────────────────────────────────────────────────────
struct AsyncAuthResult
{
    std::atomic<bool> ready{ false };
    AuthResult  result  = AuthResult::UnknownError;
    std::string message;
};

// ─── AuthManager ─────────────────────────────────────────────────────────────
class AuthManager
{
public:
    // ── Configurações KeyAuth 1.3 ──
    static constexpr const char* kName    = "Natalismonteiro2's Application";
    static constexpr const char* kOwner   = "LwlTMUzKJV";
    static constexpr const char* kVersion = "1.0";
    static constexpr const char* kUrl     = "https://keyauth.win/api/1.3/";

    // ── Parâmetros de lockout local ──
    static constexpr int   kMaxFails     = 5;    // tentativas antes de bloquear
    static constexpr DWORD kLockMs       = 30000; // 30s de bloqueio

    // ── Estado público (lido pela UI thread) ──
    AuthPage          page      = AuthPage::Login;
    AsyncAuthResult   async;
    std::string       errorMsg;
    std::string       loggedUser;
    std::string       loggedRole;
    std::string       subExpiry;
    bool              initOk    = false;

    // ── Campos de lockout (lidos pela UI) ──
    int  failCount     = 0;
    DWORD lockUntil   = 0;

    // ── Spinners / estado de request ──
    std::atomic<bool> requestInProgress{ false };

    // ─────────────────────────────────────────────────────────────────────────
    // Init (deve ser chamado uma vez antes de login/register)
    // Retorna true em sucesso.
    // ─────────────────────────────────────────────────────────────────────────
    bool Initialize()
    {
        if (requestInProgress.load() || async.ready.load()) return false;
        initOk = false;
        sessionid_.clear();
        std::string resp = PerformRequest(BuildInitFields());
        try
        {
            auto j = nlohmann::json::parse(resp);
            if (j[xorstr("success")].get<bool>())
            {
                sessionid_ = j[xorstr("sessionid")].get<std::string>();
                if (sessionid_.empty()) return false;
                initOk = true;
                return true;
            }
        }
        catch (...) {}
        initOk = false;
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Login assíncrono. Chama callback(result) na main thread via async.
    // ─────────────────────────────────────────────────────────────────────────
    void LoginAsync(const std::string& username, const std::string& password)
    {
        if (!CanAttempt()) return;

        async.ready   = false;
        async.result  = AuthResult::UnknownError;
        async.message.clear();
        requestInProgress.store(true);

        std::string user = Trim(username);
        std::string pass = password; // não trim password

        std::thread([this, user, pass]()
        {
            AuthResult r = DoLogin(user, pass);
            {
                async.result  = r;
                async.message = FriendlyMessage(r);
                async.ready   = true;
            }
            requestInProgress.store(false);
        }).detach();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Register assíncrono.
    // ─────────────────────────────────────────────────────────────────────────
    void RegisterAsync(const std::string& username,
                       const std::string& password,
                       const std::string& license)
    {
        if (!CanAttempt()) return;

        async.ready   = false;
        async.result  = AuthResult::UnknownError;
        async.message.clear();
        requestInProgress.store(true);

        std::string user = Trim(username);
        std::string pass = password;
        std::string key  = TrimEdges(license); // só edges, não interno

        std::thread([this, user, pass, key]()
        {
            AuthResult r = DoRegister(user, pass, key);
            {
                async.result  = r;
                async.message = FriendlyMessage(r);
                async.ready   = true;
            }
            requestInProgress.store(false);
        }).detach();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Chamado pela UI a cada frame após checar async.ready.
    // ─────────────────────────────────────────────────────────────────────────
    void ConsumeResult()
    {
        if (!async.ready.load() || requestInProgress.load()) return;

        if (async.result == AuthResult::Ok)
        {
            failCount = 0;
            lockUntil = 0;
            page      = AuthPage::Authenticated;
            errorMsg.clear();
        }
        else
        {
            RecordFail();
            errorMsg = async.message;
        }

        async.ready = false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Logout limpa todo o estado sensível.
    // ─────────────────────────────────────────────────────────────────────────
    void Logout()
    {
        if (requestInProgress.load()) return;
        async.ready = false;
        page       = AuthPage::Login;
        initOk     = false;
        loggedUser.clear();
        loggedRole.clear();
        subExpiry.clear();
        errorMsg.clear();
        sessionid_.clear();
        failCount  = 0;
        lockUntil  = 0;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Helpers de estado
    // ─────────────────────────────────────────────────────────────────────────
    bool IsAuthenticated() const { return page == AuthPage::Authenticated; }
    bool IsLocked()        const { return GetTickCount() < lockUntil; }

    // Segundos restantes de lockout (0 se não bloqueado).
    int LockRemainingSec() const
    {
        if (!IsLocked()) return 0;
        DWORD now = GetTickCount();
        return (int)((lockUntil - now + 999) / 1000);
    }

    // Validação local de campos (sem request).
    bool ValidateLogin(const char* user, const char* pass, std::string& outErr) const
    {
        if (!user || user[0] == '\0' || IsBlank(user))
        { outErr = "Username cannot be empty."; return false; }
        if (strnlen_s(user, 65) > 64)
        { outErr = "Username is too long."; return false; }
        if (!pass || pass[0] == '\0')
        { outErr = "Password cannot be empty."; return false; }
        if (strnlen_s(pass, 257) > 256)
        { outErr = "Password is too long."; return false; }
        return true;
    }

    bool ValidateRegister(const char* user, const char* pass, const char* confirm,
                          const char* key, std::string& outErr) const
    {
        if (!ValidateLogin(user, pass, outErr)) return false;
        if (!confirm || confirm[0] == '\0')
        { outErr = "Please confirm your password."; return false; }
        if (strcmp(pass, confirm) != 0)
        { outErr = "Passwords do not match."; return false; }
        if (!key || key[0] == '\0' || IsBlank(key))
        { outErr = "License key cannot be empty."; return false; }
        return true;
    }

    static std::string FriendlyMessage(AuthResult r)
    {
        switch (r)
        {
        case AuthResult::Ok:                  return "Login successful.";
        case AuthResult::InvalidCredentials:  return "Invalid username or password.";
        case AuthResult::HwidMismatch:        return "This account is linked to another device.\nContact support to reset your HWID.";
        case AuthResult::InvalidLicense:      return "Invalid license key.";
        case AuthResult::ExpiredLicense:      return "Your license has expired.";
        case AuthResult::LicenseAlreadyUsed:  return "This license key has already been used.";
        case AuthResult::UserExists:          return "Username already exists.";
        case AuthResult::NoInternet:          return "Unable to connect to authentication server.\nCheck your internet connection.";
        case AuthResult::SubscriptionExpired: return "Subscription expired.";
        case AuthResult::NotInitialized:      return "Authentication not initialized. Please restart.";
        case AuthResult::Locked:              return "Too many attempts. Please wait.";
        default:                              return "Authentication error. Please try again.";
        }
    }

private:
    std::string sessionid_;

    // ── HWID via SID (sem ATL) ──
    static std::string GetHwid()
    {
        HANDLE hToken = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            return "none";

        DWORD sz = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &sz);
        if (sz == 0) { CloseHandle(hToken); return "none"; }

        std::vector<BYTE> buf(sz);
        if (!GetTokenInformation(hToken, TokenUser, buf.data(), sz, &sz))
        { CloseHandle(hToken); return "none"; }

        CloseHandle(hToken);

        auto* tu = reinterpret_cast<PTOKEN_USER>(buf.data());
        LPWSTR pSid = nullptr;
        if (!ConvertSidToStringSidW(tu->User.Sid, &pSid))
            return "none";

        // wstring -> string (ASCII-safe: SID só tem dígitos/hífens)
        std::wstring ws(pSid);
        LocalFree(pSid);
        return std::string(ws.begin(), ws.end());
    }

    // ── Perform request via curl ──
    static std::string PerformRequest(const std::string& postfields)
    {
        std::string response;
        CURL* hnd = curl_easy_init();
        if (!hnd) return xorstr("connection_failed");

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, xorstr("Content-Type: application/x-www-form-urlencoded"));

        curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, xorstr("POST"));
        curl_easy_setopt(hnd, CURLOPT_URL,           xorstr("https://keyauth.win/api/1.3/"));
        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER,    headers);
        curl_easy_setopt(hnd, CURLOPT_POSTFIELDS,    postfields.c_str());
        curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(hnd, CURLOPT_WRITEDATA,     &response);
        curl_easy_setopt(hnd, CURLOPT_TIMEOUT,       10L);
        curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(hnd, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
        curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 1L); // NUNCA desabilitar SSL

        CURLcode ret = curl_easy_perform(hnd);
        long status = 0;
        curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(hnd);

        if (ret != CURLE_OK || status != 200)
            return xorstr("connection_failed");

        return response;
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        constexpr size_t limit = 256 * 1024;
        if (!userp || (size && nmemb > limit / size)) return 0;
        const size_t bytes = size * nmemb;
        auto& response = *static_cast<std::string*>(userp);
        if (response.size() > limit || bytes > limit - response.size()) return 0;
        try { response.append(static_cast<char*>(contents), bytes); }
        catch (...) { return 0; }
        return bytes;
    }

    // ── Build fields ──
    std::string BuildInitFields() const
    {
        return std::string(xorstr("type=init&ver=")) + AuthPolicy::FormEncode(kVersion) +
               xorstr("&name=")    + AuthPolicy::FormEncode(kName) +
               xorstr("&ownerid=") + AuthPolicy::FormEncode(kOwner);
    }

    std::string BuildLoginFields(const std::string& user, const std::string& pass) const
    {
        return std::string(xorstr("type=login&username=")) + AuthPolicy::FormEncode(user) +
               xorstr("&pass=")      + AuthPolicy::FormEncode(pass) +
               xorstr("&hwid=")      + AuthPolicy::FormEncode(GetHwid()) +
               xorstr("&sessionid=") + AuthPolicy::FormEncode(sessionid_) +
               xorstr("&name=")      + AuthPolicy::FormEncode(kName) +
               xorstr("&ownerid=")   + AuthPolicy::FormEncode(kOwner);
    }

    std::string BuildRegisterFields(const std::string& user, const std::string& pass,
                                    const std::string& key) const
    {
        return std::string(xorstr("type=register&username=")) + AuthPolicy::FormEncode(user) +
               xorstr("&pass=")      + AuthPolicy::FormEncode(pass) +
               xorstr("&key=")       + AuthPolicy::FormEncode(key) +
               xorstr("&hwid=")      + AuthPolicy::FormEncode(GetHwid()) +
               xorstr("&sessionid=") + AuthPolicy::FormEncode(sessionid_) +
               xorstr("&name=")      + AuthPolicy::FormEncode(kName) +
               xorstr("&ownerid=")   + AuthPolicy::FormEncode(kOwner);
    }

    // ── Core login ──
    AuthResult DoLogin(const std::string& user, const std::string& pass)
    {
        if (!initOk || sessionid_.empty())
            return AuthResult::NotInitialized;

        std::string raw = PerformRequest(BuildLoginFields(user, pass));
        if (raw == "connection_failed")
            return AuthResult::NoInternet;

        try
        {
            auto j = nlohmann::json::parse(raw);
            bool ok = j[xorstr("success")].get<bool>();
            std::string msg = j.value(xorstr("message"), "");

            if (ok)
            {
                AuthPolicy::License license;
                if (!AuthPolicy::ParseLicense(j, static_cast<std::int64_t>(time(nullptr)), license))
                    return AuthResult::SubscriptionExpired;
                loggedUser = license.user;
                loggedRole = license.subscription;
                subExpiry = license.expiry;
                return AuthResult::Ok;
            }

            return ClassifyMessage(msg);
        }
        catch (...)
        {
            return AuthResult::NoInternet;
        }
    }

    // ── Core register ──
    AuthResult DoRegister(const std::string& user, const std::string& pass,
                          const std::string& key)
    {
        if (!initOk || sessionid_.empty())
            return AuthResult::NotInitialized;

        std::string raw = PerformRequest(BuildRegisterFields(user, pass, key));
        if (raw == "connection_failed")
            return AuthResult::NoInternet;

        try
        {
            auto j  = nlohmann::json::parse(raw);
            bool ok = j[xorstr("success")].get<bool>();
            std::string msg = j.value(xorstr("message"), "");

            if (ok)
            {
                // Registration alone is not authorization: validate through login.
                return DoLogin(user, pass);
            }

            return ClassifyMessage(msg);
        }
        catch (...)
        {
            return AuthResult::NoInternet;
        }
    }

    // ── Classifica mensagem da API em AuthResult ──
    static AuthResult ClassifyMessage(const std::string& msg)
    {
        // KeyAuth 1.3 — mensagens conhecidas
        if (msg.find("invaliduser") != std::string::npos ||
            msg.find("Invalid username") != std::string::npos ||
            msg.find("Invalid password") != std::string::npos ||
            msg.find("incorrect") != std::string::npos ||
            msg.find("wrong") != std::string::npos)
            return AuthResult::InvalidCredentials;

        if (msg.find("hwid") != std::string::npos ||
            msg.find("HWID") != std::string::npos ||
            msg.find("another machine") != std::string::npos ||
            msg.find("different") != std::string::npos)
            return AuthResult::HwidMismatch;

        if (msg.find("expire") != std::string::npos &&
            msg.find("license") != std::string::npos)
            return AuthResult::ExpiredLicense;

        if (msg.find("already used") != std::string::npos ||
            msg.find("key has been used") != std::string::npos ||
            msg.find("used key") != std::string::npos)
            return AuthResult::LicenseAlreadyUsed;

        if (msg.find("invalid") != std::string::npos &&
            (msg.find("key") != std::string::npos || msg.find("license") != std::string::npos))
            return AuthResult::InvalidLicense;

        if (msg.find("user already") != std::string::npos ||
            msg.find("username already") != std::string::npos ||
            msg.find("exists") != std::string::npos)
            return AuthResult::UserExists;

        if (msg.find("subscri") != std::string::npos &&
            msg.find("expire") != std::string::npos)
            return AuthResult::SubscriptionExpired;

        if (msg.find("rate") != std::string::npos ||
            msg.find("too many") != std::string::npos ||
            msg.find("slow") != std::string::npos)
            return AuthResult::RateLimit;

        return AuthResult::InvalidCredentials; // default para falha de credencial
    }

    // ── Lockout local ──
    bool CanAttempt()
    {
        if (requestInProgress.load() || async.ready.load()) return false;
        if (IsLocked())               return false;
        return true;
    }

    void RecordFail()
    {
        failCount++;
        if (failCount >= kMaxFails)
            lockUntil = GetTickCount() + kLockMs;
    }

    // ── Utilitários ──
    static bool IsBlank(const char* s)
    {
        if (!s) return true;
        while (*s) { if (*s != ' ' && *s != '\t') return false; s++; }
        return true;
    }

    static std::string Trim(const std::string& s)
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    static std::string TrimEdges(const std::string& s)
    {
        // só remove espaços nas pontas, preserva conteúdo interno
        size_t a = s.find_first_not_of(" \t");
        if (a == std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t");
        return s.substr(a, b - a + 1);
    }
};

// ── Instância global ──
inline AuthManager g_Auth;
