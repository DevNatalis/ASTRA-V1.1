#pragma once
// UpdateManager: async update channel worker for svchost.exe.
// NEVER blocks the render thread: Check/Download run on workers, the UI
// polls a snapshot each frame. Trust model:
//   1. Manifest + binary travel over TLS (peer + host verified, HTTPS only).
//   2. The binary hash must match the manifest (integrity).
//   3. The manifest signature (embedded RSA key) must verify over
//      version/sha256/url (authenticity — the ONLY trust root).
// The installed exe is only touched by the separate Updater.exe, which
// re-verifies everything, keeps a .bak rollback copy and restarts the app.
#include <Core/Update/UpdateConfig.hpp>
#include <Core/Update/UpdateCrypto.hpp>
#include <Core/Update/UpdateManifest.hpp>
#include <Security/Api/curl/curl.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>

namespace Update {

enum class UpdateState {
    Idle,       // never checked this run
    Checking,   // manifest request in flight
    UpToDate,   // installed == latest
    Available,  // newer signed version ready to download
    Downloading,
    Verifying,
    Ready,      // staged + verified, waiting for install click
    Installing, // updater launched, app should exit
    Error
};

struct UpdateSnapshot {
    UpdateState state = UpdateState::Idle;    std::string installed;   // e.g. "1.0.0"
    std::string available;   // remote version when known
    std::string notes;
    std::string status;      // service status display
    std::string error;
    float progress = 0.f;    // 0..1 while downloading
    uint64_t doneBytes = 0, totalBytes = 0;
    bool mandatory = false;  // panel start is blocked until updated
    std::string mandatoryReason;
};

class UpdateManager {
public:
    // Integration-test hook: overrides the manifest endpoint for this
    // process. Production code never calls this — when empty, the compiled
    // ChannelConfig::ManifestUrl() is used. Set once before workers start.
    static void SetManifestUrlOverride(const std::string& url) {
        ManifestUrlOverride() = url;
    }

    UpdateSnapshot Poll() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return snap_;
    }

    bool IsBusy() const {
        UpdateState s = State();
        return s == UpdateState::Checking || s == UpdateState::Downloading ||
               s == UpdateState::Verifying || s == UpdateState::Installing;
    }

    // Starts a manifest check unless one is already running.
    void CheckAsync() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (workerBusy_) return;
        if (snap_.state == UpdateState::Checking ||
            snap_.state == UpdateState::Downloading ||
            snap_.state == UpdateState::Verifying)
            return;
        workerBusy_ = true;
        snap_.state = UpdateState::Checking;
        snap_.error.clear();
        snap_.installed = InstalledVersionStr();
        std::thread([this]() { CheckWorker(); }).detach();
    }

    // Requests cancellation of an in-flight download. The worker deletes
    // the partial staging file and returns to the Available state.
    void Cancel() { cancel_.store(true); }

    // Downloads the staged binary (only from Available state).
    void DownloadAsync() {        std::lock_guard<std::mutex> lock(mutex_);
        if (workerBusy_ || snap_.state != UpdateState::Available) return;
        workerBusy_ = true;
        snap_.state = UpdateState::Downloading;
        snap_.progress = 0.f;
        snap_.doneBytes = snap_.totalBytes = 0;
        cancel_.store(false);
        std::thread([this]() { DownloadWorker(); }).detach();
    }

    // Launches Updater.exe (only from Ready state). Returns false when the
    // updater binary is missing or the state is wrong.
    bool InstallAsync() {
        Manifest manifest;
        std::wstring staged, target;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (snap_.state != UpdateState::Ready) return false;
            manifest = manifest_;
            staged = stagedPath_;
            if (staged.empty()) return false;
        }
        wchar_t self[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, self, MAX_PATH)) return false;
        std::wstring dir(self);
        size_t sep = dir.find_last_of(L"\\/");
        dir = (sep == std::wstring::npos) ? L"." : dir.substr(0, sep);
        const std::wstring updater = dir + L"\\Updater.exe";
        if (GetFileAttributesW(updater.c_str()) == INVALID_FILE_ATTRIBUTES) {
            Fail("Updater.exe was not found next to the application.");
            return false;
        }
        const std::string payload = manifest.Payload();
        const std::string cmd =
            "\"" + WideToUtf8(updater) + "\""
            " --wait-pid " + std::to_string(GetCurrentProcessId()) +
            " --input \"" + WideToUtf8(staged) + "\""
            " --target \"" + WideToUtf8(dir + L"\\svchost.exe") + "\""
            " --expect-sha256 " + manifest.sha256 +
            " --payload \"" + Crypto::Base64Encode(
                                   reinterpret_cast<const unsigned char*>(payload.data()),
                                   payload.size()) +
            "\""
            " --signature \"" + manifest.signature + "\""
            " --run-after";
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        std::wstring wcmd = Utf8ToWide(cmd);
        if (!CreateProcessW(updater.c_str(), wcmd.data(), nullptr, nullptr, FALSE,
                CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            Fail("Could not start Updater.exe.");
            return false;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snap_.state = UpdateState::Installing;
        }
        return true;
    }

    // Panel gate: true while a mandatory update is pending.
    bool MandatoryPending() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return snap_.mandatory;
    }

private:
    static constexpr long kManifestTimeoutSec = 15;
    static constexpr uint64_t kManifestCap = 65536;
    static constexpr uint64_t kBinaryCap = 200ULL * 1024ULL * 1024ULL;

    static std::string& ManifestUrlOverride() {
        static std::string url;
        return url;
    }

    mutable std::mutex mutex_;
    UpdateSnapshot snap_;
    Manifest manifest_;
    std::wstring stagedPath_;
    bool workerBusy_ = false;
    std::atomic<bool> cancel_{ false };

    UpdateState State() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return snap_.state;
    }

    void Fail(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        snap_.state = UpdateState::Error;
        snap_.error = msg;
        workerBusy_ = false;
    }

    static void CurlInitOnce() {
        static std::once_flag flag;
        std::call_once(flag, []() { curl_global_init(CURL_GLOBAL_ALL); });
    }

    static size_t BodyCallback(void* c, size_t size, size_t n, void* user) {
        size_t bytes = size * n;
        auto* body = static_cast<std::string*>(user);
        if (body->size() + bytes > kManifestCap) return 0; // abort: too large
        try {
            body->append(static_cast<char*>(c), bytes);
        } catch (...) { return 0; }
        return bytes;
    }

    // Common TLS-hardened handle: peer + host verified, HTTPS only.
    static bool ApplyTls(CURL* h, long timeoutSec, bool followRedirects) {
        bool ok = true;
        ok = (curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_REDIR_PROTOCOLS_STR, "https") == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, followRedirects ? 1L : 0L) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_MAXREDIRS, 5L) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_TIMEOUT, timeoutSec) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 10L) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L) == CURLE_OK) && ok;
        return ok;
    }

    void CheckWorker() {
        CurlInitOnce();
        std::string url = ManifestUrlOverride();
        if (url.empty())
            url = ChannelConfig::ManifestUrl();
        if (!IsAllowedHttpsUrl(url)) {
            Fail("Update endpoint is not a valid HTTPS URL.");
            return;
        }
        std::string body;
        CURL* h = curl_easy_init();
        if (!h) {
            Fail("Could not start the update check.");
            return;
        }
        bool ok = ApplyTls(h, kManifestTimeoutSec, true);
        ok = (curl_easy_setopt(h, CURLOPT_URL, url.c_str()) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, BodyCallback) == CURLE_OK) && ok;
        ok = (curl_easy_setopt(h, CURLOPT_WRITEDATA, &body) == CURLE_OK) && ok;
        long status = 0;
        CURLcode rc = ok ? curl_easy_perform(h) : CURLE_FAILED_INIT;
        if (rc == CURLE_OK)
            curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_cleanup(h);
        if (rc != CURLE_OK || (status != 0 && status != 200) || body.empty()) {
            Fail("Update server is unreachable. Check your connection.");
            return;
        }
        Manifest m = Manifest::Parse(body);
        if (!m.ok) {
            Fail(std::string("Invalid update manifest: ") + m.error);
            return;
        }
        if (!m.VerifySignature()) {
            Fail("Manifest signature check failed. Update rejected.");
            return;
        }
        const SemVer installed = SemVer::Parse(InstalledVersionStr());
        const int cmp = installed.Compare(m.version);
        std::lock_guard<std::mutex> lock(mutex_);
        snap_.installed = InstalledVersionStr();
        snap_.available = m.version.Str();
        snap_.notes = m.notes;
        snap_.status = m.status.empty() ? ChannelConfig::ServiceStatusDefault() : m.status;
        manifest_ = m;
        // Mandatory when the channel says so for a newer build, or when the
        // installed build is below the channel floor.
        bool belowFloor = m.minVersion.valid && installed.Compare(m.minVersion) < 0;
        snap_.mandatory = (m.mandatory && cmp < 0) || belowFloor;
        if (snap_.mandatory) {
            snap_.mandatoryReason = belowFloor && cmp >= 0
                ? "This build is below the minimum allowed version."
                : "A mandatory update is pending.";
        } else {
            snap_.mandatoryReason.clear();
        }
        snap_.state = (cmp < 0) ? UpdateState::Available : UpdateState::UpToDate;
        workerBusy_ = false;
    }

    struct FileCtx {
        FILE* f = nullptr;
        uint64_t written = 0;
        UpdateManager* self = nullptr;
    };

    static size_t FileCallback(void* c, size_t size, size_t n, void* user) {
        size_t bytes = size * n;
        FileCtx* ctx = static_cast<FileCtx*>(user);
        if (ctx->written + bytes > kBinaryCap) return 0; // abort: too large
        if (bytes && fwrite(c, 1, bytes, ctx->f) != bytes) return 0;
        ctx->written += bytes;
        return bytes;
    }

    static int ProgressCallback(void* user, curl_off_t total, curl_off_t done,
            curl_off_t /*ulTotal*/, curl_off_t /*ulDone*/) {
        UpdateManager* self = static_cast<UpdateManager*>(user);
        if (self->cancel_.load()) return 1; // abort transfer
        std::lock_guard<std::mutex> lock(self->mutex_);
        self->snap_.doneBytes = (uint64_t)done;
        self->snap_.totalBytes = (uint64_t)total;
        self->snap_.progress = (total > 0) ? (float)((double)done / (double)total) : 0.f;
        return 0;
    }

    // Fixed staging name inside %TEMP% — the remote URL path is NEVER used
    // as a filename (path traversal safe) and staging never lands in a
    // sensitive directory.
    static std::wstring StagingPath() {
        wchar_t tmp[MAX_PATH]{};
        DWORD n = GetTempPathW(MAX_PATH, tmp);
        std::wstring dir = (n > 0 && n < MAX_PATH)
            ? std::wstring(tmp) + L"svchost-update"
            : std::wstring(L".\\svchost-update");
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\svchost-update.exe";
    }

    void DownloadWorker() {
        CurlInitOnce();
        Manifest m;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            m = manifest_;
        }
        if (!m.ok || !IsAllowedHttpsUrl(m.url)) {
            Fail("Update target URL is invalid.");
            return;
        }
        const std::wstring staged = StagingPath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, staged.c_str(), L"wb") != 0 || !f) {
            Fail("Could not write the update staging file.");
            return;
        }
        FileCtx ctx{};
        ctx.f = f;
        ctx.self = this;
        CURL* h = curl_easy_init();
        bool started = (h != nullptr);
        if (started) {
            bool ok = ApplyTls(h, 0L, true); // bulk transfer: no total timeout
            ok = (curl_easy_setopt(h, CURLOPT_LOW_SPEED_LIMIT, 1024L) == CURLE_OK) && ok;
            ok = (curl_easy_setopt(h, CURLOPT_LOW_SPEED_TIME, 30L) == CURLE_OK) && ok;
            ok = (curl_easy_setopt(h, CURLOPT_URL, m.url.c_str()) == CURLE_OK) && ok;
            ok = (curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, FileCallback) == CURLE_OK) && ok;
            ok = (curl_easy_setopt(h, CURLOPT_WRITEDATA, &ctx) == CURLE_OK) && ok;
            ok = (curl_easy_setopt(h, CURLOPT_XFERINFOFUNCTION, ProgressCallback) == CURLE_OK) && ok;
            ok = (curl_easy_setopt(h, CURLOPT_XFERINFODATA, this) == CURLE_OK) && ok;
            ok = (curl_easy_setopt(h, CURLOPT_NOPROGRESS, 0L) == CURLE_OK) && ok;
            CURLcode rc = ok ? curl_easy_perform(h) : CURLE_FAILED_INIT;
            long status = 0;
            if (rc == CURLE_OK)
                curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &status);
            curl_easy_cleanup(h);
            started = (rc == CURLE_OK && (status == 0 || status == 200));
        }
        fclose(f);
        if (cancel_.load()) {
            DeleteFileW(staged.c_str());
            std::lock_guard<std::mutex> lock(mutex_);
            snap_.state = UpdateState::Available; // back to pre-download
            snap_.progress = 0.f;
            workerBusy_ = false;
            return;
        }
        if (!started) {
            DeleteFileW(staged.c_str());
            Fail("Download failed. The previous version is untouched.");
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snap_.state = UpdateState::Verifying;
        }
        // Integrity: hash must match the SIGNED manifest field.
        unsigned char digest[32];
        if (!Crypto::Sha256File(staged, digest, kBinaryCap) ||
            Crypto::HexOf(digest, 32) != m.sha256) {
            DeleteFileW(staged.c_str());
            Fail("Downloaded file failed the integrity check.");
            return;
        }
        // Authenticity already proven by the signed manifest; re-verify the
        // signature over the staged payload inputs as defense in depth.
        if (!m.VerifySignature()) {
            DeleteFileW(staged.c_str());
            Fail("Downloaded file failed the signature check.");
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stagedPath_ = staged;
            snap_.state = UpdateState::Ready;
            snap_.progress = 1.f;
            workerBusy_ = false;
        }
    }

    static std::string WideToUtf8(const std::wstring& w) {
        if (w.empty()) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (n <= 1) return {};
        std::string out((size_t)n - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), n, nullptr, nullptr);
        return out;
    }

    static std::wstring Utf8ToWide(const std::string& s) {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        if (n <= 1) return {};
        std::wstring out((size_t)n - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
        return out;
    }
};

inline UpdateManager& Manager() {
    static UpdateManager instance;
    return instance;
}

} // namespace Update
