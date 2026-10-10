// Updater.exe — standalone installer for the svchost update channel.
// Launched by svchost.exe AFTER the user confirms install; the app exits and
// this process performs the swap. No ImGui, no curl, no network: everything
// was already downloaded + verified by the main app, and is re-verified
// here before touching the installed executable.
//
// Protocol (argv):
//   Updater.exe --wait-pid <N> --input <staged> --target <exe>
//               --expect-sha256 <hex64> --payload <base64> --signature <base64>
//               [--run-after]
// Steps: wait -> already-current? -> verify staged -> backup -> swap ->
// verify -> rollback on mismatch -> optional relaunch.
// Exit codes: 0 ok, 1 failure (previous version kept).
#include <Updater/UpdaterCore.hpp>

#include <string>

namespace {

void Log(const char* m) {
    printf("[updater] %s\n", m);
    fflush(stdout);
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (n <= 1) return {};
    std::wstring out((size_t)n - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
    return out;
}

std::string StripQuotes(std::string s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

struct Args {
    DWORD waitPid = 0;
    std::wstring input, target;
    std::string expectSha256, payloadB64, signatureB64;
    bool runAfter = false;
    bool ok = false;
};

Args ParseArgs(int argc, char** argv) {
    Args a;
    auto take = [&](int& i) -> std::string {
        if (i + 1 >= argc) return {};
        return StripQuotes(argv[++i]);
    };
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--wait-pid") a.waitPid = (DWORD)strtoul(take(i).c_str(), nullptr, 10);
        else if (k == "--input") a.input = Utf8ToWide(take(i));
        else if (k == "--target") a.target = Utf8ToWide(take(i));
        else if (k == "--expect-sha256") a.expectSha256 = take(i);
        else if (k == "--payload") a.payloadB64 = take(i);
        else if (k == "--signature") a.signatureB64 = take(i);
        else if (k == "--run-after") a.runAfter = true;
    }
    a.ok = a.waitPid != 0 && !a.input.empty() && !a.target.empty() &&
           a.expectSha256.size() == 64 && !a.payloadB64.empty() &&
           !a.signatureB64.empty();
    return a;
}

} // namespace

int main(int argc, char** argv) {
    Args a = ParseArgs(argc, argv);
    if (!a.ok) {
        Log("bad arguments");
        return 1;
    }
    return UpdaterCore::Install(a.waitPid, a.input, a.target,
        a.expectSha256, a.payloadB64, a.signatureB64, a.runAfter, Log);
}
