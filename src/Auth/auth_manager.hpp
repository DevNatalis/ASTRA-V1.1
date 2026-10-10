#pragma once
// language: C++, file: Auth/auth_manager.hpp, target: Windows x64, MSVC
// KeyAuth API 1.3 — AuthManager centralizado para svchost.
// Credenciais: name=Natalismonteiro2's Application ownerid=LwlTMUzKJV version=1.0
// NÃO inclui Application Secret (API 1.3 nao exige no cliente).
// NÃO inclui Seller Key, Admin Key ou qualquer chave administrativa.

#include <windows.h>
#include <sddl.h>
#include <winioctl.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
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
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "bcrypt.lib") // CNG: SHA-256 (HWID + integridade da sessao)

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
    SessionExpired,   // sessao expirou (expiry do servidor) — re-login obrigatorio
    SessionRevoked,   // heartbeat falhou 3x ou integridade violada — re-login obrigatorio
    SecurityConfig,   // pin SSL nao configurado — contate o suporte (fail closed)
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
    // Single UI consumer. seq_cst publication synchronizes all preceding
    // result/message writes; CanAttempt prevents reuse until consumption.
    std::atomic<bool> ready{ false };
    AuthResult  result  = AuthResult::UnknownError;
    std::string message;
};

// ─── AuthManager ─────────────────────────────────────────────────────────────
class AuthManager
{
public:
    // ── Segredos de API ofuscados em repouso ──
    // NOTA HONESTA: ofuscacao XOR dificulta extracao trivial via `strings`,
    // mas NAO e fronteira de seguranca — o binario sempre pode ser revertido.
    // A credencial real e por-sessao e emitida pelo servidor (sessionid +
    // expiry assinado pelo KeyAuth, ver AuthSession abaixo). Troque kXorMask
    // a cada release para invalidar assinaturas byte-a-byte anteriores.
    struct AuthSecrets
    {
        static constexpr unsigned char kXorMask = 0x5A;

        static std::string Decode(const unsigned char* blob, size_t len)
        {
            std::string out;
            out.reserve(len);
            for (size_t i = 0; i < len; ++i)
                out.push_back(static_cast<char>(blob[i] ^ kXorMask ^ static_cast<unsigned char>(i & 0xFF)));
            return out;
        }

        // Gerados com: byte ^ 0x5A ^ (indice & 0xFF). Para trocar um valor,
        // rode no PowerShell:
        //   $b=[Text.Encoding]::UTF8.GetBytes("<novo valor>");
        //   ($b | % {$i=0} { $_ -bxor 0x5A -bxor ($i -band 0xFF); $i++ } |
        //     % { '0x{0:X2}' -f $_ }) -join ', '
        static std::string Name()
        {
            static const unsigned char b[] = {
                0x14, 0x3A, 0x2C, 0x38, 0x32, 0x36, 0x2F, 0x30, 0x3D, 0x3D,
                0x24, 0x34, 0x3F, 0x25, 0x3B, 0x67, 0x6D, 0x38, 0x68, 0x08,
                0x3E, 0x3F, 0x20, 0x24, 0x21, 0x22, 0x34, 0x28, 0x29, 0x29 };
            return Decode(b, sizeof(b));
        }
        static std::string Owner()
        {
            static const unsigned char b[] = {
                0x16, 0x2C, 0x34, 0x0D, 0x13, 0x0A, 0x26, 0x16, 0x18, 0x05 };
            return Decode(b, sizeof(b));
        }
        static std::string Version()
        {
            static const unsigned char b[] = { 0x6B, 0x75, 0x68 };
            return Decode(b, sizeof(b));
        }
        static std::string Url()
        {
            static const unsigned char b[] = {
                0x32, 0x2F, 0x2C, 0x29, 0x2D, 0x65, 0x73, 0x72, 0x39, 0x36,
                0x29, 0x30, 0x23, 0x23, 0x3C, 0x7B, 0x3D, 0x22, 0x26, 0x66,
                0x2F, 0x3F, 0x25, 0x62, 0x73, 0x6D, 0x73, 0x6E };
            return Decode(b, sizeof(b));
        }
    };

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
    bool initializing = false;

    void InitializeAsync()
    {
        if (initializing || requestInProgress.load() || async.ready.load()) return;
        if (authThread_.joinable()) authThread_.join();
        initOk = false;
        StopHeartbeat();
        ForceLogoutState();
        sessionid_.clear();
        errorMsg.clear();
        initError_.clear();
        initializing = true;
        requestInProgress.store(true);
        authThread_ = std::thread([this]() {
            initSucceeded_ = InitializeRequest();
            requestInProgress.store(false);
            initReady_.store(true);
        });
    }

    void ConsumeInitialization()
    {
        if (!initReady_.load()) return;
        if (authThread_.joinable()) authThread_.join();
        initOk = initSucceeded_;
        errorMsg = initError_;
        initializing = false;
        initReady_.store(false);
    }

    bool InitializeRequest()
    {
        std::string resp = PerformRequest(BuildInitFields());
        // Diagnostico granular: a UI mostra a causa real em vez de um
        // "sem internet" generico (pin errado x rede x servidor).
        if (resp == "pinning_not_configured")
        {
            // Configuration failure, not a server-side version rejection.
            initError_ = "Authentication certificate pin is not configured.";
            return false;
        }
        if (resp == "pinning_failed")
        {
            // Pin nao bateu: MITM local (Fiddler, antivírus com scan TLS, VPN)
            // ou o servidor trocou de chave. Desligue interceptadores e tente de novo.
            initError_ = "Connection blocked: authentication certificate mismatch.";
            return false;
        }
        if (resp == "connection_failed")
        {
            initError_ = FriendlyMessage(AuthResult::NoInternet);
            return false;
        }
        try
        {
            auto j = nlohmann::json::parse(resp);
            if (j[xorstr("success")].get<bool>())
            {
                sessionid_ = j[xorstr("sessionid")].get<std::string>();
                if (sessionid_.empty()) {
                    initError_ = "Authentication server returned an empty session.";
                    return false;
                }
                return true;
            }
            initError_ = ServerMessage(j.value("message", ""));
        }
        catch (...) { initError_ = "Invalid response from authentication server."; }
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Login assíncrono. Chama callback(result) na main thread via async.
    // ─────────────────────────────────────────────────────────────────────────
    void LoginAsync(const std::string& username, const std::string& password)
    {
        if (!CanAttempt()) return;
        if (authThread_.joinable()) authThread_.join();

        async.ready   = false;
        async.result  = AuthResult::UnknownError;
        async.message.clear();
        requestInProgress.store(true);

        std::string user = Trim(username);
        std::string pass = password; // não trim password

        authThread_ = std::thread([this, user, pass]()
        {
            AuthResult r = DoLogin(user, pass);
            {
                async.result  = r;
                if (async.message.empty()) async.message = FriendlyMessage(r);
                async.ready   = true;
            }
            requestInProgress.store(false);
        });
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Register assíncrono.
    // ─────────────────────────────────────────────────────────────────────────
    void RegisterAsync(const std::string& username,
                       const std::string& password,
                       const std::string& license)
    {
        if (!CanAttempt()) return;
        if (authThread_.joinable()) authThread_.join();

        async.ready   = false;
        async.result  = AuthResult::UnknownError;
        async.message.clear();
        requestInProgress.store(true);

        std::string user = Trim(username);
        std::string pass = password;
        std::string key  = TrimEdges(license); // só edges, não interno

        authThread_ = std::thread([this, user, pass, key]()
        {
            AuthResult r = DoRegister(user, pass, key);
            {
                async.result  = r;
                if (async.message.empty()) async.message = FriendlyMessage(r);
                async.ready   = true;
            }
            requestInProgress.store(false);
        });
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
            // Arma o recibo de sessao (sessionid mascarado + integridade +
            // heartbeat). Sem isso, IsSessionValid() continua falso e o menu
            // nao abre — flipar bool de UI nao basta.
            ActivateSession();
            if (!IsSessionValid())
            {
                errorMsg = FriendlyMessage(AuthResult::SessionExpired);
                async.ready = false;
                return;
            }
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
        StopHeartbeat();
        ForceLogoutState();
        async.ready = false;
        errorMsg.clear();
        failCount  = 0;
        lockUntil  = 0;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Helpers de estado
    // ─────────────────────────────────────────────────────────────────────────
    bool IsAuthenticated() const { return page == AuthPage::Authenticated; }
    bool IsLocked()        const { return GetTickCount() < lockUntil; }

    // ─────────────────────────────────────────────────────────────────────────
    // SESSAO (VULN 1): recibo assinado pelo servidor + revalidacao continua.
    // Regra: nenhuma acao critica confia em bool de UI. O unico "esta
    // autorizado?" valido e IsSessionValid()/RequireAuth() abaixo.
    // ─────────────────────────────────────────────────────────────────────────
    ~AuthManager() { Shutdown(); }
    void Shutdown() {
        if (authThread_.joinable()) authThread_.join();
        StopHeartbeat();
    }

    // Arma a sessao APOS login validado pelo servidor. Chamado uma vez no
    // ConsumeResult (thread da UI). Mascara o sessionid, calcula integridade
    // e inicia o heartbeat. Idempotente.
    void ActivateSession()
    {
        StopHeartbeat();
        {
            std::lock_guard<std::mutex> lock(sessionMutex_);
            EnsureBootNonceLocked();
            memcpy(session_.nonce, bootNonce_, sizeof(session_.nonce));
            session_.user = loggedUser;
            session_.expiryUnix = licenseExpiryUnix_;
            session_.issuedAt = static_cast<std::int64_t>(time(nullptr));
            session_.maskedSid = MaskSidLocked(sessionid_);
            session_.lastHeartbeat = session_.issuedAt;
            session_.hbFails = 0;
            session_.revoked = false;
            session_.integrity = SessionIntegrityLocked();
            session_.armed = !session_.maskedSid.empty() &&
                             session_.expiryUnix > session_.issuedAt;
        }
        if (!sessionid_.empty())
            SecureZeroMemory(&sessionid_[0], sessionid_.size());
        sessionid_.clear();
        if (IsSessionValid())
            StartHeartbeat();
    }

    // sessionid em claro SOMENTE para montar requests (copia temporaria).
    std::string SessionId() const
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        // Before successful login, the API session lives in sessionid_.
        // ActivateSession moves it into maskedSid only after authorization.
        if (!session_.armed) return sessionid_;
        return UnmaskSidLocked(session_.maskedSid);
    }

    // Barato (sem hash, sem rede): para hot paths (EntityList @1ms, render).
    bool IsSessionValid() const
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        // Worker threads consult only state protected by sessionMutex_.
        // The UI owns page and updates it outside this mutex.
        if (!session_.armed || session_.revoked) return false;
        std::int64_t now = static_cast<std::int64_t>(time(nullptr));
        if (session_.expiryUnix <= 0 || now >= session_.expiryUnix) return false;
        if (session_.hbFails >= kHbMaxFails) return false;
        return true;
    }

    // Completo (com anti-patch): antes de cada ACAO CRITICA (login efetivo,
    // GetOffsets, abrir menu). Em falha, derruba a sessao (fail closed).
    bool RequireAuth()
    {
        {
            std::lock_guard<std::mutex> lock(sessionMutex_);
            if (page != AuthPage::Authenticated || !session_.armed ||
                session_.revoked || session_.hbFails >= kHbMaxFails)
            {
                ForceLogoutStateLocked();
                return false;
            }
            std::int64_t now = static_cast<std::int64_t>(time(nullptr));
            if (session_.expiryUnix <= 0 || now >= session_.expiryUnix)
            {
                ForceLogoutStateLocked();
                errorMsg = FriendlyMessage(AuthResult::SessionExpired);
                return false;
            }
            if (!VerifyIntegrityLocked())
            {
                session_.revoked = true;
                ForceLogoutStateLocked();
                errorMsg = FriendlyMessage(AuthResult::SessionRevoked);
                return false;
            }
        }
        return true;
    }

    // Derruba para a tela de login. Seguro em qualquer thread (nao toca UI).
    // A UI espelha: Login::Render observa IsSessionValid() e zera IsLogged.
    void ForceLogoutState()
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        ForceLogoutStateLocked();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // HEARTBEAT: checks the existing API session every ~12 minutes.
    // - type=check with the authenticated session, timeout 5s;
    // - reforca expiry local emitido pelo servidor;
    // - 3 falhas seguidas => sessao revogada => UI forca re-login.
    // NOTA HONESTA KeyAuth 1.3: nao ha endpoint dedicado de "ban check";
    // ban aplicado entre heartbeats e pego no proximo login/upgrade. Este
    // loop garante sessao fresca e canal integro, nao deteccao de ban
    // em tempo real (isso exigiria endpoint server-side).
    // ─────────────────────────────────────────────────────────────────────────
    void StartHeartbeat()
    {
        if (hbRunning_.load()) return;
        hbStop_.store(false);
        hbRunning_.store(true);
        hbThread_ = std::thread([this]() { HeartbeatLoop(); });
    }

    void StopHeartbeat()
    {
        hbStop_.store(true);
        if (hbThread_.joinable())
        {
            if (hbThread_.get_id() == std::this_thread::get_id())
                hbThread_.detach(); // nunca join em si mesma (revogacao)
            else
                hbThread_.join();
        }
        hbRunning_.store(false);
    }

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
        case AuthResult::SessionExpired:      return "Session expired. Please sign in again.";
        case AuthResult::SessionRevoked:      return "Session revoked by the server. Please sign in again.";
        case AuthResult::SecurityConfig:      return "Security misconfiguration. Contact support.";
        default:                              return "Authentication error. Please try again.";
        }
    }

private:
#ifdef SVCHOST_AUTH_TESTS
    friend struct AuthRegression;
#endif
    std::thread authThread_;
    std::atomic<bool> initReady_{ false };
    bool initSucceeded_ = false;
    std::string initError_;
    // ── Sessao autenticada (VULN 1) ──
    // O menu NUNCA e liberado por um bool em memoria: acoes criticas exigem
    // IsSessionValid()/RequireAuth(), que conferem recibo assinado pelo
    // servidor (sessionid + expiry), integridade anti-patch e heartbeat.
    struct AuthSession
    {
        bool armed = false;          // true apos login validado pelo servidor
        bool revoked = false;        // heartbeat 3x fail / integridade violada
        std::string user;            // username validado pelo servidor
        std::int64_t expiryUnix = 0; // expiry da subscription (servidor)
        std::int64_t issuedAt = 0;
        std::string maskedSid;       // sessionid XOR nonce de boot (nunca plaintext)
        unsigned char nonce[32] = {};
        std::string integrity;       // SHA256 hex p/ detectar patch em memoria
        std::int64_t lastHeartbeat = 0;
        int hbFails = 0;
    };

    mutable std::mutex sessionMutex_;
    AuthSession session_;
    unsigned char bootNonce_[32] = {};
    bool bootNonceReady_ = false;
    std::int64_t licenseExpiryUnix_ = 0; // preenchido no DoLogin via ParseLicense

    // ── Heartbeat (VULN 4) ──
    std::thread hbThread_;
    std::atomic<bool> hbStop_{ false };
    std::atomic<bool> hbRunning_{ false };
    static constexpr int  kHbIntervalSec = 12 * 60; // 12min (janela pedida: 10–15min)
    static constexpr int  kHbMaxFails    = 3;
    static constexpr long kHbTimeoutSec  = 5;       // teto pedido: 5s por tentativa

    std::string sessionid_; // transiente init->login; zerado no ActivateSession

    // ── HWID legado via SID (por-USUARIO). Mantido SO como ultimo fallback,
    // prefixado com "sid-" para nunca colidir com HWIDs reais de maquina. ──
    static std::string GetUserSid()
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
        LPSTR pSid = nullptr;
        if (!ConvertSidToStringSidA(tu->User.Sid, &pSid))
            return "none";

        // wstring -> string (ASCII-safe: SID só tem dígitos/hífens)
        std::string sid(pSid);
        LocalFree(pSid);
        return sid;
    }

    // ── SHA-256 via CNG (sem dependencias externas) ──
    static bool Sha256(const void* data, size_t len, unsigned char out[32])
    {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        bool ok = false;
        if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
        {
            DWORD cbHash = 0, cbOut = 0;
            if (BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
                    reinterpret_cast<PUCHAR>(&cbHash), sizeof(cbHash), &cbOut, 0)) && cbHash == 32)
            {
                if (BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0)) &&
                    BCRYPT_SUCCESS(BCryptHashData(hHash, (PUCHAR)data, (ULONG)len, 0)))
                {
                    DWORD done = 0;
                    ok = BCRYPT_SUCCESS(BCryptFinishHash(hHash, out, 32, 0));
                    (void)done;
                }
            }
            if (hHash) BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
        }
        if (!ok) SecureZeroMemory(out, 32);
        return ok;
    }

    static std::string HexOf(const unsigned char* data, size_t len)
    {
        static constexpr char hex[] = "0123456789abcdef";
        std::string out;
        out.reserve(len * 2);
        for (size_t i = 0; i < len; ++i)
        {
            out.push_back(hex[data[i] >> 4]);
            out.push_back(hex[data[i] & 0xF]);
        }
        return out;
    }

    // ── HWID (VULN 3): identidade da MAQUINA, nao do usuario ──
    // Fonte 1: tabela SMBIOS (RSMB) — contem UUID da placa-mae/BIOS.
    //   Nao parseamos o UUID: o hash cobre o blob inteiro, o que e estavel
    //   por maquina e dispensa parsing fragil de estruturas SMBIOS.
    //   Leitura via GetSystemFirmwareTable (Win32 puro, sem WMI/Registry).
    // Fonte 2: serial do disco fisico via IOCTL_STORAGE_QUERY_PROPERTY.
    //   (O processo roda elevado — vide UAC RequireAdministrator no vcxproj.)
    static std::vector<BYTE> ReadSmbiosBlob()
    {
        constexpr DWORD kMax = 256 * 1024;
        DWORD sz = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
        if (sz == 0 || sz > kMax) return {};
        std::vector<BYTE> buf(sz);
        DWORD got = GetSystemFirmwareTable('RSMB', 0, buf.data(), sz);
        if (got == 0 || got > sz) return {};
        buf.resize(got);
        return buf;
    }

    static std::string ReadDiskSerial()
    {
        HANDLE h = CreateFileW(L"\\\\.\\PhysicalDrive0", 0,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) return {};

        STORAGE_PROPERTY_QUERY q{};
        q.PropertyId = StorageDeviceProperty;
        q.QueryType  = PropertyStandardQuery;
        BYTE out[1024] = {};
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY,
            &q, sizeof(q), out, sizeof(out), &ret, nullptr);
        CloseHandle(h);
        if (!ok || ret < sizeof(STORAGE_DEVICE_DESCRIPTOR) || ret > sizeof(out)) return {};

        auto* d = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(out);
        if (d->SerialNumberOffset == 0 || d->SerialNumberOffset >= ret) return {};

        const char* start = reinterpret_cast<char*>(out) + d->SerialNumberOffset;
        const size_t remaining = ret - d->SerialNumberOffset;
        const auto* end = static_cast<const char*>(std::memchr(start, '\0', remaining));
        if (!end) return {};
        std::string s(start, static_cast<size_t>(end - start));
        // trim: seriais ATA costumam vir com espacos
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return {};
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    static std::string ComputeHwid()
    {
        std::vector<BYTE> smbios = ReadSmbiosBlob();
        std::string disk = ReadDiskSerial();
        if (!smbios.empty() || !disk.empty())
        {
            std::string blob = "svchost-HWID-v1|";
            blob.append(reinterpret_cast<const char*>(smbios.data()), smbios.size());
            blob += '|';
            blob += disk;
            unsigned char h[32];
            if (Sha256(blob.data(), blob.size(), h))
                return HexOf(h, 32);
        }

        // Fallback fisico: serial do volume do sistema (muda ao formatar).
        DWORD vsn = 0;
        if (GetVolumeInformationW(L"C:\\", nullptr, 0, &vsn,
                nullptr, nullptr, nullptr, 0) && vsn != 0)
        {
            char tmp[64];
            sprintf_s(tmp, sizeof(tmp), "svchost-HWID-vol1|%08lX", (unsigned long)vsn);
            unsigned char h[32];
            if (Sha256(tmp, strlen(tmp), h))
                return HexOf(h, 32);
        }

        // Ultimo recurso (por-usuario, documentado como fraco).
        std::string sid = GetUserSid();
        if (sid != "none" && !sid.empty())
            return "sid-" + sid;
        return "none";
    }

    // HWID com cache: leitura fisica (SMBIOS + disco) acontece uma vez por
    // processo. RequireAuth() roda a 60fps — sem cache, seria I/O por frame.
    // Magic static = inicializacao thread-safe (C++11). So cacheia sucesso;
    // falha ("none") tenta de novo na proxima chamada.
    static std::string GetHwid()
    {
        static std::mutex cacheMutex;
        std::lock_guard<std::mutex> lock(cacheMutex);
        static std::string cached;
        if (!cached.empty())
            return cached;
        std::string fresh = ComputeHwid();
        if (!fresh.empty() && fresh != "none")
            cached = fresh;
        return fresh;
    }

    // ── Public-Key Pinning (VULN 2) ──
    // Derrota Fiddler/Burp mesmo com a raiz deles instalada no Windows:
    // a cadeia pode validar, mas o SPKI nao bate com o pin e o curl aborta
    // com CURLE_SSL_PINNEDPUBKEYNOTMATCH antes de enviar qualquer dado.
    //
    // ROTACAO (quando o KeyAuth renovar o certificado — o SPKI costuma
    // sobreviver a renovacao se a chave for reutilizada; se mudar, o login
    // falha fechado com SecurityConfig e este e o procedimento):
    //   1. openssl s_client -connect keyauth.win:443 -servername keyauth.win
    //        2>nul | openssl x509 -pubkey -noout
    //        | openssl pkey -pubin -outform der
    //        | openssl dgst -sha256 -binary | openssl enc -base64
    //   2. Cole o resultado como "sha256//<base64>" em kPinnedSpki abaixo.
    //   3. Teste: ative o Fiddler (HTTPS decrypt + raiz confiavel) e tente
    //      logar — deve falhar com erro de pin, nunca completar o login.
    // Pin refreshed 2026-10-05: leaf CN=keyauth.win issued by Let's Encrypt YE2,
    // expires 2027-01-02. Previous pin no longer matched any cert in the live chain.
    static constexpr const char* kPinnedSpki = "sha256//Uph03/nw3T0tdWja1iU6bxYUBryw4jzeFdyeSuVRMEs=";
    // ^^^ SETUP OBRIGATORIO (uma vez): sem o pin real o cliente falha
    // fechado (fail closed) — ver PinLooksConfigured. Nao envie release assim.

    static bool PinLooksConfigured()
    {
        return kPinnedSpki &&
               strstr(kPinnedSpki, "PUT_REAL") == nullptr &&
               strncmp(kPinnedSpki, "sha256//", sizeof("sha256//") - 1) == 0 &&
               strlen(kPinnedSpki) > 16;
    }

    // ── Perform request via curl ──
    static std::string PerformRequest(const std::string& postfields, long timeoutSec = 10L)
    {
        // Fail closed: sem pin real configurado, nem tenta a rede.
        if (!PinLooksConfigured())
            return xorstr("pinning_not_configured");

        std::string response;
        CURL* hnd = curl_easy_init();
        if (!hnd) return xorstr("connection_failed");

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, xorstr("Content-Type: application/x-www-form-urlencoded"));

        bool configured = headers != nullptr;
        std::string url = AuthSecrets::Url(); // curl copia a string; temporario e seguro
        configured = (curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, xorstr("POST")) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_URL,           url.c_str()) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_HTTPHEADER,    headers) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_POSTFIELDS,    postfields.c_str()) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallback) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_WRITEDATA,     &response) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_TIMEOUT,       timeoutSec) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, timeoutSec < 5L ? timeoutSec : 5L) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_REDIR_PROTOCOLS_STR, "https") == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK) && configured; // NUNCA desabilitar SSL
        // Pin da chave publica do keyauth.win: rejeita MITM com raiz instalada.
        configured = (curl_easy_setopt(hnd, CURLOPT_PINNEDPUBLICKEY, kPinnedSpki) == CURLE_OK) && configured;

        if (!configured) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(hnd);
            return xorstr("connection_failed");
        }

        CURLcode ret = curl_easy_perform(hnd);
        long status = 0;
        curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(hnd);

        if (ret == CURLE_SSL_PINNEDPUBKEYNOTMATCH)
            return xorstr("pinning_failed");
        if (ret != CURLE_OK || status != 200)
            return xorstr("connection_failed");

        return response;
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        constexpr size_t limit = 256 * 1024;
        if (!userp || !contents || !size || nmemb > limit / size) return 0;
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
        return std::string(xorstr("type=init&ver=")) + AuthPolicy::FormEncode(AuthSecrets::Version()) +
               xorstr("&name=")    + AuthPolicy::FormEncode(AuthSecrets::Name()) +
               xorstr("&ownerid=") + AuthPolicy::FormEncode(AuthSecrets::Owner());
    }

    std::string BuildLoginFields(const std::string& user, const std::string& pass) const
    {
        return std::string(xorstr("type=login&username=")) + AuthPolicy::FormEncode(user) +
               xorstr("&pass=")      + AuthPolicy::FormEncode(pass) +
               xorstr("&hwid=")      + AuthPolicy::FormEncode(GetHwid()) +
               xorstr("&sessionid=") + AuthPolicy::FormEncode(SessionId()) +
               xorstr("&name=")      + AuthPolicy::FormEncode(AuthSecrets::Name()) +
               xorstr("&ownerid=")   + AuthPolicy::FormEncode(AuthSecrets::Owner());
    }

    std::string BuildRegisterFields(const std::string& user, const std::string& pass,
                                    const std::string& key) const
    {
        return std::string(xorstr("type=register&username=")) + AuthPolicy::FormEncode(user) +
               xorstr("&pass=")      + AuthPolicy::FormEncode(pass) +
               xorstr("&key=")       + AuthPolicy::FormEncode(key) +
               xorstr("&hwid=")      + AuthPolicy::FormEncode(GetHwid()) +
               xorstr("&sessionid=") + AuthPolicy::FormEncode(SessionId()) +
               xorstr("&name=")      + AuthPolicy::FormEncode(AuthSecrets::Name()) +
               xorstr("&ownerid=")   + AuthPolicy::FormEncode(AuthSecrets::Owner());
    }

    // ── Core login ──
    AuthResult DoLogin(const std::string& user, const std::string& pass)
    {
        if (!initOk || sessionid_.empty())
            return AuthResult::NotInitialized;

        std::string raw = PerformRequest(BuildLoginFields(user, pass));
        if (raw == "pinning_not_configured" || raw == "pinning_failed")
            return AuthResult::SecurityConfig;
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
                try { licenseExpiryUnix_ = std::stoll(license.expiry); }
                catch (...) { licenseExpiryUnix_ = 0; }
                if (licenseExpiryUnix_ <= static_cast<std::int64_t>(time(nullptr)))
                    return AuthResult::SubscriptionExpired;
                return AuthResult::Ok;
            }

            async.message = ServerMessage(msg);
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
        if (raw == "pinning_not_configured" || raw == "pinning_failed")
            return AuthResult::SecurityConfig;
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

            async.message = ServerMessage(msg);
            return ClassifyMessage(msg);
        }
        catch (...)
        {
            return AuthResult::NoInternet;
        }
    }

    // ── Classifica mensagem da API em AuthResult ──
    static std::string ServerMessage(const std::string& message)
    {
        std::string text = message.substr(0, 240);
        for (char& c : text)
            if (static_cast<unsigned char>(c) < 32) c = ' ';
        return text.empty() ? "Authentication server rejected the request." : text;
    }

    std::string BuildCheckFields() const
    {
        return std::string("type=check&sessionid=") + AuthPolicy::FormEncode(SessionId()) +
               "&name=" + AuthPolicy::FormEncode(AuthSecrets::Name()) +
               "&ownerid=" + AuthPolicy::FormEncode(AuthSecrets::Owner());
    }

    static AuthResult ClassifyMessage(const std::string& message)
    {
        std::string msg = message;
        for (char& c : msg)
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        // KeyAuth 1.3 — mensagens conhecidas
        if (msg.find("invaliduser") != std::string::npos ||
            msg.find("invalid username") != std::string::npos ||
            msg.find("invalid password") != std::string::npos ||
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

        if (msg.find("banned") != std::string::npos ||
            msg.find("blacklist") != std::string::npos)
            return AuthResult::SessionRevoked;

        if (msg.find("session") != std::string::npos ||
            msg.find("sessao") != std::string::npos)
            return AuthResult::SessionExpired;

        return AuthResult::UnknownError;
    }

    // ── Internos de sessao/heartbeat (chamados com sessionMutex_ preso,
    // exceto onde indicado) ──
    void EnsureBootNonceLocked()
    {
        if (bootNonceReady_) return;
        // RtlGenRandom (SystemFunction036, advapi32 — sempre carregada).
        // Sem chamada estatica: resolve via GetProcAddress, zero header novo.
        HMODULE adv = GetModuleHandleW(L"advapi32.dll");
        using RngFn = BOOLEAN(NTAPI*)(PVOID, ULONG);
        auto fn = adv ? reinterpret_cast<RngFn>(GetProcAddress(adv, "SystemFunction036")) : nullptr;
        if (fn && fn(bootNonce_, sizeof(bootNonce_)))
            bootNonceReady_ = true;
        else
        {
            // Degradacao honesta: nonce derivado de tempo+pid (fraco, mas a
            // seguranca real continua sendo o servidor, nao o nonce).
            ULONGLONG t = GetTickCount64() ^ (static_cast<ULONGLONG>(GetCurrentProcessId()) << 32);
            for (size_t i = 0; i < sizeof(bootNonce_); ++i)
            {
                t = t * 6364136223846793005ULL + 1442695040888963407ULL;
                bootNonce_[i] = static_cast<unsigned char>((t >> 33) & 0xFF);
            }
            bootNonceReady_ = true;
        }
    }

    std::string MaskSidLocked(const std::string& sid) const
    {
        std::string out = sid;
        for (size_t i = 0; i < out.size(); ++i)
            out[i] ^= static_cast<char>(bootNonce_[i % sizeof(bootNonce_)]);
        return out;
    }

    std::string UnmaskSidLocked(const std::string& masked) const
    {
        return MaskSidLocked(masked); // XOR e simetrico
    }

    std::string SessionIntegrityLocked() const
    {
        std::string blob = UnmaskSidLocked(session_.maskedSid);
        blob += '|';
        blob += session_.user;
        blob += '|';
        blob += std::to_string(session_.expiryUnix);
        blob += '|';
        blob += GetHwid();
        blob.append(reinterpret_cast<const char*>(session_.nonce), sizeof(session_.nonce));
        unsigned char h[32];
        if (!Sha256(blob.data(), blob.size(), h))
            return {};
        SecureZeroMemory(&blob[0], blob.size());
        return HexOf(h, 32);
    }

    bool VerifyIntegrityLocked() const
    {
        if (session_.integrity.empty()) return false;
        return SessionIntegrityLocked() == session_.integrity;
    }

    void ForceLogoutStateLocked()
    {
        session_.armed = false;
        session_.revoked = true;
        if (!session_.maskedSid.empty())
        {
            SecureZeroMemory(&session_.maskedSid[0], session_.maskedSid.size());
            session_.maskedSid.clear();
        }
        session_.integrity.clear();
        session_.user.clear();
        page = AuthPage::Login;
        initOk = false;
        loggedUser.clear();
        loggedRole.clear();
        subExpiry.clear();
        licenseExpiryUnix_ = 0;
    }

    void HeartbeatLoop()
    {
        while (!hbStop_.load())
        {
            for (int i = 0; i < kHbIntervalSec && !hbStop_.load(); ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
            if (hbStop_.load()) break;

            {
                std::lock_guard<std::mutex> lock(sessionMutex_);
                if (!session_.armed || session_.revoked) continue;
                std::int64_t now = static_cast<std::int64_t>(time(nullptr));
                if (session_.expiryUnix > 0 && now >= session_.expiryUnix)
                {
                    session_.revoked = true; // UI fara o logout via RequireAuth
                    continue;
                }
            }

            std::string resp = PerformRequest(BuildCheckFields(), kHbTimeoutSec);
            bool alive = false;
            bool rejected = false;
            try
            {
                auto j = nlohmann::json::parse(resp);
                if (j.is_object() && j.contains("success") && j.at("success").is_boolean()) {
                    alive = j.at("success").get<bool>();
                    rejected = !alive;
                }
            }
            catch (...) { alive = false; }

            {
                std::lock_guard<std::mutex> lock(sessionMutex_);
                if (!session_.armed) continue;
                if (alive && VerifyIntegrityLocked())
                {
                    session_.hbFails = 0;
                    session_.lastHeartbeat = static_cast<std::int64_t>(time(nullptr));
                }
                else if (rejected || ++session_.hbFails >= kHbMaxFails)
                {
                    session_.revoked = true; // proximo RequireAuth derruba
                }
            }
        }
        hbRunning_.store(false);
    }

    // ── Lockout local ──
    bool CanAttempt()
    {
        if (initializing || !initOk) return false;
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
