#pragma once
// UpdaterCore: the install/rollback engine shared by Updater.exe and the
// integration test. Header-only, Windows + UpdateCrypto only (no ImGui,
// no curl, no network).
//
// Contract:
//   Install() returns 0 on success. Any failure returns 1 AND leaves the
//   previous installation runnable: nothing is touched before the staged
//   file passes hash + signature, and a post-swap mismatch restores .bak.
#include <Core/Update/UpdateConfig.hpp>
#include <Core/Update/UpdateCrypto.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace UpdaterCore {

inline bool HashMatches(const std::wstring& path, const std::string& expectHex) {
    unsigned char digest[32];
    if (!Update::Crypto::Sha256File(path, digest)) return false;
    std::string got = Update::Crypto::HexOf(digest, 32);
    if (got.size() != expectHex.size()) return false;
    unsigned diff = 0; // constant-time compare
    for (size_t i = 0; i < got.size(); ++i)
        diff |= (unsigned)(got[i] ^ expectHex[i]);
    return diff == 0;
}

inline bool SignatureOk(const std::string& payloadB64, const std::string& sigB64) {
    std::vector<unsigned char> payload, sig;
    if (!Update::Crypto::Base64Decode(payloadB64, payload)) return false;
    if (!Update::Crypto::Base64Decode(sigB64, sig)) return false;
    // NOTE: fetch key parts in sequenced statements — inlining the length
    // out-params in the call below would read them in unspecified order
    // (lengths seen as 0, verification always failing).
    size_t expLen = 0, modLen = 0;
    const unsigned char* exp = Update::UpdatePublicKey::Exponent(expLen);
    const unsigned char* mod = Update::UpdatePublicKey::Modulus(modLen);
    return Update::Crypto::RsaPkcs1Sha256Verify(mod, modLen, exp, expLen,
        payload.data(), payload.size(), sig.data(), sig.size());
}

inline bool WaitForExit(DWORD pid, DWORD timeoutMs) {
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!h) return true; // already gone: proceed
    DWORD w = WaitForSingleObject(h, timeoutMs);
    CloseHandle(h);
    return w != WAIT_FAILED;
}

inline bool BackupFile(const std::wstring& target, const std::wstring& bak) {
    DeleteFileW(bak.c_str());
    return CopyFileW(target.c_str(), bak.c_str(), TRUE) != FALSE;
}

inline bool SwapFile(const std::wstring& input, const std::wstring& target) {
    // COPY_ALLOWED: staging lives in %TEMP%, which may sit on another
    // volume than the install dir (RAM disk, redirected temp, ...).
    // Without it, cross-volume moves fail with ERROR_NOT_SAME_DEVICE.
    return MoveFileExW(input.c_str(), target.c_str(),
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED |
                   MOVEFILE_WRITE_THROUGH) != FALSE;
}

inline bool RestoreBackup(const std::wstring& bak, const std::wstring& target) {
    return MoveFileExW(bak.c_str(), target.c_str(),
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED |
                   MOVEFILE_WRITE_THROUGH) != FALSE;
}

inline bool LaunchExe(const std::wstring& exe) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + exe + L"\"";
    if (!CreateProcessW(exe.c_str(), cmd.data(), nullptr, nullptr, FALSE,
            0, nullptr, nullptr, &si, &pi))
        return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

// Full install sequence. log may be nullptr. Returns process exit code.
inline int Install(DWORD waitPid, const std::wstring& input,
        const std::wstring& target, const std::string& expectSha256,
        const std::string& payloadB64, const std::string& sigB64,
        bool runAfter, void (*log)(const char*)) {
    auto say = [&](const char* m) { if (log) log(m); };

    // Only the application executable may ever be replaced. This keeps a
    // locally-invoked updater from becoming an arbitrary-file writer: even
    // with a valid signature, nothing but svchost.exe is a legal target.
    {
        size_t sep = target.find_last_of(L"\\/");
        std::wstring base = (sep == std::wstring::npos) ? target : target.substr(sep + 1);
        if (base.size() != 9) {
            say("refusing non-application target");
            return 1;
        }
        wchar_t lower[10]{};
        for (size_t i = 0; i < 9; ++i) {
            wchar_t c = base[i];
            lower[i] = (c >= L'A' && c <= L'Z') ? (wchar_t)(c + 32) : c;
        }
        if (wcscmp(lower, L"svchost.exe") != 0) {
            say("refusing non-application target");
            return 1;
        }
    }

    if (!WaitForExit(waitPid, 60000)) {
        say("could not wait for parent process");
        return 1;
    }

    if (HashMatches(target, expectSha256)) {
        say("target already matches expected hash");
        if (runAfter && !LaunchExe(target)) return 1;
        DeleteFileW(input.c_str());
        return 0;
    }

    if (!HashMatches(input, expectSha256)) {
        say("staged file hash mismatch; keeping installed version");
        return 1;
    }
    if (!SignatureOk(payloadB64, sigB64)) {
        say("staged manifest signature invalid; keeping installed version");
        DeleteFileW(input.c_str());
        return 1;
    }

    const std::wstring bak = target + L".bak";
    if (!BackupFile(target, bak)) {
        say("could not back up installed executable");
        return 1;
    }
    if (!SwapFile(input, target)) {
        say("swap failed; backup preserved");
        return 1;
    }
    if (!HashMatches(target, expectSha256)) {
        say("installed hash mismatch after swap; rolling back");
        RestoreBackup(bak, target);
        return 1;
    }
    DeleteFileW(bak.c_str());
    say("update installed and verified");

    if (runAfter && !LaunchExe(target)) {
        say("installed, but relaunch failed");
        return 1;
    }
    return 0;
}

} // namespace UpdaterCore
