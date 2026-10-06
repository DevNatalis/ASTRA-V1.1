"""Offline checks for RELATORIO_SEGURANCA; never start ASTRA or a driver."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/security-audit/checks'
out.mkdir(parents=True, exist_ok=True)

def function(path, signature):
    source = (root / path).read_text(encoding='utf-8-sig')
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>
using SIZE_T = size_t;
using DWORD = unsigned long;
using BOOL = int;
using HANDLE = void*;
using LPVOID = void*;
constexpr int PAGE_EXECUTE_READWRITE = 64;
constexpr uintptr_t base = 0x100000;
std::vector<uint8_t> remote;
bool failRead = false;
size_t partial = (std::numeric_limits<size_t>::max)();
BOOL VirtualProtectEx(HANDLE, LPVOID, size_t, DWORD, DWORD* old) {
    assert(old); *old = 4; return 1;
}
BOOL ReadProcessMemory(HANDLE, const void* address, void* data, size_t size, SIZE_T* received) {
    *received = 0;
    if (failRead) return 0;
    const auto offset = reinterpret_cast<uintptr_t>(address) - base;
    if (offset > remote.size()) return 0;
    *received = (std::min)({size, remote.size() - offset, partial});
    std::memcpy(data, remote.data() + offset, *received);
    return 1;
}
class MemoryClass {
public:
    HANDLE ProcHandle = nullptr;
    uintptr_t ModBase = base, ModBaseSize = 0;
    uintptr_t FindSignature(std::vector<uint8_t>, uintptr_t, uintptr_t);
    uintptr_t FindSignatureBypass(std::vector<uint8_t>, uintptr_t, uintptr_t);
};
SCANNER_ONE
SCANNER_TWO
struct MEMORY_BASIC_INFORMATION { void* BaseAddress; size_t RegionSize; };
struct WildcardByte { bool is_wildcard; uint8_t value; };
DUMPER
CALLBACK
DESTINATION
int main() {
    assert(IsDiscoveryDestination("https://cfx.re/join/abc123"));
    assert(IsDiscoveryDestination("https://servers.fivem.net/servers/detail/abc123"));
    for (const char* url : {"http://cfx.re/join/abc", "https://cfx.re.evil.test/join/abc",
         "https://cfx.re@evil.test/join/abc", "https://evil.test/", "https://cfx.re/other/abc"})
        assert(!IsDiscoveryDestination(url));
    MemoryClass memory;
    for (auto method : {&MemoryClass::FindSignature, &MemoryClass::FindSignatureBypass}) {
        remote.assign(17000, 0x11);
        auto scan = [&](std::vector<uint8_t> pattern) {
            return (memory.*method)(pattern, base, remote.size());
        };
        assert(scan({}) == 0);
        assert(scan(std::vector<uint8_t>(20000, 1)) == 0);
        assert(scan({0x22, 0x33}) == 0);
        remote[remote.size()-2] = 0x22; remote.back() = 0x33;
        assert(scan({0x22, 0x33}) == base + remote.size() - 2);
        assert(scan({0x22, 0}) == base + remote.size() - 2);
        remote[16383] = 0x44; remote[16384] = 0x55;
        assert(scan({0x44, 0x55}) == base + 16383);
        partial = 1; assert(scan({0x44, 0x55}) == 0);
        partial = (std::numeric_limits<size_t>::max)();
        failRead = true; assert(scan({0x11}) == 0); failRead = false;
        assert((memory.*method)({0x11}, (std::numeric_limits<uintptr_t>::max)() - 1, 4) == 0);
        remote.assign(1, 0x22); assert(scan({0x22, 0x33}) == 0);
    }
    remote = {0x11, 0x22};
    MEMORY_BASIC_INFORMATION region{reinterpret_cast<void*>(base), remote.size()};
    assert(find_in_region(nullptr, region, {}) == 0);
    assert(find_in_region(nullptr, region, {{false, 0x11}, {false, 0x22}, {false, 0x33}}) == 0);
    partial = 1;
    assert(find_in_region(nullptr, region, {{false, 0x11}, {false, 0x22}}) == 0);
    partial = (std::numeric_limits<size_t>::max)();
    assert(find_in_region(nullptr, region, {{false, 0x22}}) == base + 1);
    failRead = true;
    assert(find_in_region(nullptr, region, {{false, 0x11}}) == 0);
    std::string body(2 * 1024 * 1024 - 1, 'a');
    char byte = 'x';
    assert(WriteCallBack(&byte, 1, 1, &body) == 1);
    assert(WriteCallBack(&byte, 1, 1, &body) == 0);
    assert(WriteCallBack(&byte, (std::numeric_limits<size_t>::max)(), 2, &body) == 0);
    assert(WriteCallBack(nullptr, 0, 1, &body) == 0);
    std::cout << "Scanners: empty, oversized, partial, failed read, end, crossing block, overflow; bounded callback: PASS\n";
}
'''
for token, path, signature in (
    ('SCANNER_ONE', 'src/Core/SDK/Memory.cpp', 'uintptr_t MemoryClass::FindSignature('),
    ('SCANNER_TWO', 'src/Core/SDK/Memory.cpp', 'uintptr_t MemoryClass::FindSignatureBypass('),
    ('DUMPER', 'fivem-offset-dumper-main/memory/memory.cpp', 'static uintptr_t find_in_region('),
    ('CALLBACK', 'src/Core/Threads/UpdateNames.hpp', 'static size_t WriteCallBack('),
    ('DESTINATION', 'src/Core/Threads/UpdateNames.hpp', 'static bool IsDiscoveryDestination('),
):
    source = source.replace(token, function(path, signature))
(out / 'boundaries.cpp').write_text(source, encoding='utf-8')
(out / 'versions.cpp').write_text(r'''
#define CURL_STATICLIB
#include <Security/Api/curl/curl.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
int main() {
    const auto* info = curl_version_info(CURLVERSION_NOW);
    std::cout << "curl header=" << LIBCURL_VERSION << " linked=" << info->version
              << " tls=" << (info->ssl_version ? info->ssl_version : "none") << '\n';
    FT_Library ft = nullptr;
    if (FT_Init_FreeType(&ft)) return 1;
    FT_Int major = 0, minor = 0, patch = 0;
    FT_Library_Version(ft, &major, &minor, &patch);
    std::cout << "FreeType header=" << FREETYPE_MAJOR << '.' << FREETYPE_MINOR << '.' << FREETYPE_PATCH
              << " linked=" << major << '.' << minor << '.' << patch << '\n';
    FT_Done_FreeType(ft);
}
''', encoding='utf-8')
vcvars = Path(r'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat')
src = root / 'src'
commands = [f'@call "{vcvars}" >nul', '@if errorlevel 1 exit /b 1']
commands += [
    '@cl /nologo /std:c++17 /EHsc /W4 /Od /RTC1 boundaries.cpp /Fe:boundaries.exe',
    '@if errorlevel 1 exit /b 1', '@boundaries.exe', '@if errorlevel 1 exit /b 1',
    f'@cl /nologo /std:c++17 /EHsc /W3 /MD /DCURL_STATICLIB /I"{src}" "{root / "tests/auth_security_checks.cpp"}" /Fe:auth_checks.exe /link /LIBPATH:"{src / "Security/Api/curl"}" advapi32.lib',
    '@if errorlevel 1 exit /b 1', '@auth_checks.exe', '@if errorlevel 1 exit /b 1',
    f'@cl /nologo /std:c++17 /EHsc /MD /I"{src}" /I"{src / "Includes/ImGui/Files/FreeType/include"}" versions.cpp /Fe:versions.exe /link /LIBPATH:"{src / "Security/Api/curl"}" /LIBPATH:"{src / "Includes/ImGui/Files/FreeType/win64"}" libcurl.lib freetype.lib ws2_32.lib normaliz.lib crypt32.lib wldap32.lib advapi32.lib secur32.lib bcrypt.lib iphlpapi.lib',
    '@if errorlevel 1 exit /b 1', '@versions.exe', '@exit /b %errorlevel%',
]
(out / 'checks.cmd').write_text('\n'.join(commands), encoding='utf-8')
result = subprocess.run(['cmd.exe', '/d', '/c', str(out / 'checks.cmd')], cwd=out, capture_output=True)
(out / 'checks.log').write_bytes(result.stdout + result.stderr)
print((result.stdout + result.stderr).decode('utf-8', errors='replace'))
raise SystemExit(result.returncode)
