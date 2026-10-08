// update_e2e: headless driver for the update end-to-end matrix.
// Modes:
//   check   --url U --expect-state S [--expect-version V] [--expect-mandatory 0|1]
//   full    --url U --expect-terminal Ready|Error [--expect-version V]
//   corelib --dir D --wait-pid N --sha H --payload-b64 P --sig-b64 S
// Prints KEY=value lines for the PowerShell orchestrator and exits 0/1.
#pragma comment(lib, "libcurl.lib")
#include <Core/Update/UpdateConfig.hpp>
#include <Core/Update/UpdateCrypto.hpp>
#include <Core/Update/UpdateManifest.hpp>
#include <Core/Update/UpdateManager.hpp>
#include <Updater/UpdaterCore.hpp>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

namespace {

using Clock = std::chrono::steady_clock;

std::string ArgVal(int argc, char** argv, const char* key) {
    for (int i = 1; i + 1 < argc; ++i)
        if (strcmp(argv[i], key) == 0) return argv[i + 1];
    return {};
}

const char* StateName(Update::UpdateState s) {
    using Update::UpdateState;
    switch (s) {
    case UpdateState::Idle: return "Idle";
    case UpdateState::Checking: return "Checking";
    case UpdateState::UpToDate: return "UpToDate";
    case UpdateState::Available: return "Available";
    case UpdateState::Downloading: return "Downloading";
    case UpdateState::Verifying: return "Verifying";
    case UpdateState::Ready: return "Ready";
    case UpdateState::Installing: return "Installing";
    case UpdateState::Error: return "Error";
    }
    return "?";
}

bool IsTerminal(Update::UpdateState s) {
    return s != Update::UpdateState::Idle && s != Update::UpdateState::Checking &&
           s != Update::UpdateState::Downloading && s != Update::UpdateState::Verifying;
}

// Waits for a non-transient state; returns the last snapshot.
Update::UpdateSnapshot WaitSettled(int timeoutSec, long& maxPollMs) {
    maxPollMs = 0;
    Update::UpdateSnapshot s;
    auto t0 = Clock::now();
    for (;;) {
        auto p0 = Clock::now();
        s = Update::Manager().Poll();
        long ms = (long)std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - p0).count();
        if (ms > maxPollMs) maxPollMs = ms;
        if (IsTerminal(s.state)) return s;
        if (std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - t0).count() >
            timeoutSec)
            return s;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int ModeCheck(int argc, char** argv) {
    std::string url = ArgVal(argc, argv, "--url");
    std::string expState = ArgVal(argc, argv, "--expect-state");
    std::string expVer = ArgVal(argc, argv, "--expect-version");
    std::string expMand = ArgVal(argc, argv, "--expect-mandatory");
    Update::UpdateManager::SetManifestUrlOverride(url);
    Update::Manager().CheckAsync();
    long maxPoll = 0;
    Update::UpdateSnapshot s = WaitSettled(30, maxPoll);
    std::cout << "STATE=" << StateName(s.state) << "\n";
    std::cout << "VERSION=" << s.available << "\n";
    std::cout << "MANDATORY=" << (s.mandatory ? 1 : 0) << "\n";
    std::cout << "REASON=" << s.mandatoryReason << "\n";
    std::cout << "ERROR=" << s.error << "\n";
    bool ok = StateName(s.state) == expState;
    if (!expVer.empty()) ok = ok && (s.available == expVer);
    if (!expMand.empty()) ok = ok && ((s.mandatory ? "1" : "0") == expMand);
    return ok ? 0 : 1;
}

int ModeFull(int argc, char** argv) {
    std::string url = ArgVal(argc, argv, "--url");
    std::string expTerminal = ArgVal(argc, argv, "--expect-terminal");
    std::string expVer = ArgVal(argc, argv, "--expect-version");
    Update::UpdateManager::SetManifestUrlOverride(url);
    Update::Manager().CheckAsync();
    long maxPoll = 0;
    Update::UpdateSnapshot s = WaitSettled(30, maxPoll);
    std::cout << "CHECK_STATE=" << StateName(s.state) << "\n";
    if (s.state == Update::UpdateState::Available) {
        if (!expVer.empty() && s.available != expVer) {
            std::cout << "VERSION=" << s.available << "\n";
            return 1;
        }
        Update::Manager().DownloadAsync();
        auto t0 = Clock::now();
        int samples = 0;
        for (;;) {
            auto p0 = Clock::now();
            s = Update::Manager().Poll();
            long ms = (long)std::chrono::duration_cast<std::chrono::milliseconds>(
                Clock::now() - p0).count();
            if (ms > maxPoll) maxPoll = ms;
            if (s.state == Update::UpdateState::Downloading) {
                ++samples;
                if (samples <= 5)
                    std::cout << "PROGRESS=" << s.progress << "\n";
            }
            if (IsTerminal(s.state)) break;
            if (std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - t0).count() > 120)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << "SAMPLES=" << samples << "\n";
    }
    std::cout << "STATE=" << StateName(s.state) << "\n";
    std::cout << "VERSION=" << s.available << "\n";
    std::cout << "MAX_POLL_MS=" << maxPoll << "\n";
    std::cout << "ERROR=" << s.error << "\n";
    return StateName(s.state) == expTerminal ? 0 : 1;
}

std::wstring ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out((size_t)(n > 1 ? n - 1 : 0), L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
    return out;
}

bool WriteFile(const std::wstring& path, const std::string& data) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    fclose(f);
    return ok;
}

std::string ReadFile(const std::wstring& path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) return {};
    std::string out;
    char buf[4096];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    fclose(f);
    return out;
}

// Step-level install/rollback verification with real file ops.
// Each case runs in its own subdirectory starting from the pristine
// installed bytes, so the already-current short-circuit of one case can
// never mask another.
int ModeCoreLib(int argc, char** argv) {
    std::string dir = ArgVal(argc, argv, "--dir");
    DWORD waitPid = (DWORD)strtoul(ArgVal(argc, argv, "--wait-pid").c_str(), nullptr, 10);
    std::string sha = ArgVal(argc, argv, "--sha");
    std::string payload = ArgVal(argc, argv, "--payload-b64");
    std::string sig = ArgVal(argc, argv, "--sig-b64");
    if (dir.empty() || sha.size() != 64 || payload.empty() || sig.empty()) {
        std::cout << "CORELIB=bad-args\n";
        return 1;
    }
    const std::string contentA = "APP-v1.0.0-installed-content";
    const std::string contentB = "APP-v1.1.0-staged-content";
    int failures = 0;
    auto setup = [&](const char* sub, const std::string& targetData,
                     const std::string& stagedData) -> std::wstring {
        std::wstring W = ToWide(dir + std::string("\\") + sub);
        CreateDirectoryW(W.c_str(), nullptr);
        WriteFile(W + L"\\ASTRA.exe", targetData);
        if (!stagedData.empty())
            WriteFile(W + L"\\staged.exe", stagedData);
        else
            DeleteFileW((W + L"\\staged.exe").c_str());
        return W;
    };

    // Case A: full Install() success path (real functions, real files).
    {
        std::wstring W = setup("a", contentA, contentB);
        int rc = UpdaterCore::Install(waitPid, W + L"\\staged.exe", W + L"\\ASTRA.exe",
            sha, payload, sig, false, nullptr);
        bool ok = (rc == 0) && (ReadFile(W + L"\\ASTRA.exe") == contentB) &&
                  (GetFileAttributesW((W + L"\\ASTRA.exe.bak").c_str()) ==
                   INVALID_FILE_ATTRIBUTES);
        std::cout << "CORELIB_INSTALL=" << (ok ? "ok" : "FAIL") << "\n";
        if (!ok) ++failures;
    }

    // Case B: tampered staged file -> exit 1, installed build untouched.
    {
        std::wstring W = setup("b", contentA, contentB + "TAMPERED");
        int rc = UpdaterCore::Install(waitPid, W + L"\\staged.exe", W + L"\\ASTRA.exe",
            sha, payload, sig, false, nullptr);
        bool ok = (rc != 0) && (ReadFile(W + L"\\ASTRA.exe") == contentA);
        std::cout << "CORELIB_TAMPER=" << (ok ? "ok" : "FAIL") << "\n";
        if (!ok) ++failures;
    }

    // Case C: forged signature -> exit 1, installed build untouched.
    {
        std::wstring W = setup("c", contentA, contentB);
        std::string badSig = sig;
        badSig[10] = (badSig[10] == 'A' ? 'B' : 'A');
        int rc = UpdaterCore::Install(waitPid, W + L"\\staged.exe", W + L"\\ASTRA.exe",
            sha, payload, badSig, false, nullptr);
        bool ok = (rc != 0) && (ReadFile(W + L"\\ASTRA.exe") == contentA);
        std::cout << "CORELIB_FAKESIG=" << (ok ? "ok" : "FAIL") << "\n";
        if (!ok) ++failures;
    }

    // Case D: rollback â€” backup, swap, corrupt target, verify fails,
    // RestoreBackup must bring back the exact original bytes.
    {
        std::wstring W = setup("d", contentA, contentB);
        bool okD = UpdaterCore::BackupFile(W + L"\\ASTRA.exe", W + L"\\ASTRA.exe.bak") &&
                   UpdaterCore::SwapFile(W + L"\\staged.exe", W + L"\\ASTRA.exe") &&
                   WriteFile(W + L"\\ASTRA.exe", contentB + "CORRUPT-POST-SWAP") &&
                   !UpdaterCore::HashMatches(W + L"\\ASTRA.exe", sha) &&
                   UpdaterCore::RestoreBackup(W + L"\\ASTRA.exe.bak", W + L"\\ASTRA.exe") &&
                   (ReadFile(W + L"\\ASTRA.exe") == contentA);
        std::cout << "CORELIB_ROLLBACK=" << (okD ? "ok" : "FAIL") << "\n";
        if (!okD) ++failures;
    }

    std::cout << "CORELIB_DONE failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: update_e2e (check|full|corelib) ...\n";
        return 2;
    }
    std::string mode = argv[1];
    if (mode == "check") return ModeCheck(argc, argv);
    if (mode == "full") return ModeFull(argc, argv);
    if (mode == "corelib") return ModeCoreLib(argc, argv);
    std::cerr << "unknown mode\n";
    return 2;
}
