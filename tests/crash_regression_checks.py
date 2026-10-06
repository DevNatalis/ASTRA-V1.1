"""Compile isolated source excerpts with Win32 mocks; never run the application."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
utils = (root / 'src/Includes/Utils.hpp').read_text(encoding='utf-8-sig')
clipboard = utils.split('inline  std::string GetClipboard( ) {', 1)[1].split('\n\tinline void PasteClipboard', 1)[0]
registry = (root / 'src/Core/Threads/UpdateNames.hpp').read_text(encoding='utf-8-sig')
registry = registry.split('char value[ 255 ]', 1)[1].split('DirFiveM = ( std::string ) value;', 1)[0]
source = r'''
#include <string>
#include <cassert>
#include <cstring>
#include <iostream>
using HANDLE = void*;
using SIZE_T = size_t;
using DWORD = unsigned long;
constexpr int CF_TEXT = 1, HKEY_CURRENT_USER = 1, RRF_RT_REG_SZ = 1, ERROR_SUCCESS = 0;
bool openOk, dataOk, lockOk;
int closes, unlocks;
char data[8];
SIZE_T capacity = sizeof(data);
bool OpenClipboard(void*) { return openOk; }
bool CloseClipboard() { ++closes; return true; }
HANDLE GetClipboardData(int) { return dataOk ? data : nullptr; }
SIZE_T GlobalSize(HANDLE) { return capacity; }
void* GlobalLock(HANDLE) { return lockOk ? data : nullptr; }
bool GlobalUnlock(HANDLE) { ++unlocks; return true; }
const char* xorstr(const char* s) { return s; }
DWORD required;
int RegGetValueA(int, const char*, const char*, int, void*, void* out, DWORD* size) {
    assert(*size == 255);
    if (required > *size) { *size = required; return 234; }
    std::memset(out, 'a', required - 1);
    static_cast<char*>(out)[required - 1] = 0;
    *size = required;
    return 0;
}
std::string GetClipboard() {
''' + clipboard + '\nstd::string ReadRegistry() { char value[ 255 ]' + registry + r'''
return std::string(value);
}
int main() {
    for (int scenario = 0; scenario < 6; ++scenario) {
        openOk = scenario != 0; dataOk = scenario != 1; lockOk = scenario != 2;
        closes = unlocks = 0;
        std::memset(data, 0, sizeof(data));
        if (scenario == 4) std::memcpy(data, "hello", 5);
        if (scenario == 5) std::memset(data, 'x', sizeof(data));
        assert(GetClipboard() == (scenario == 4 ? "hello" : ""));
        assert(closes == (openOk ? 1 : 0));
        assert(unlocks == (scenario >= 3 ? 1 : 0));
    }
    capacity = 1024 * 1024 + 1; closes = unlocks = 0;
    assert(GetClipboard().empty()); assert(closes == 1 && unlocks == 0);
    required = 255; assert(ReadRegistry().size() == 254);
    required = 256; assert(ReadRegistry().empty());
    required = 8192; assert(ReadRegistry().empty());
    std::cout << "10 regression cases passed\n";
}
'''
vcvars = Path(r'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat')
with tempfile.TemporaryDirectory(prefix='astra-crash-') as temp:
    work = Path(temp)
    (work / 'checks.cpp').write_text(source, encoding='utf-8')
    (work / 'build.cmd').write_text(
        f'@call "{vcvars}" >nul\n@cl /nologo /std:c++17 /EHsc /W4 checks.cpp /Fe:checks.exe\n@exit /b %errorlevel%\n',
        encoding='utf-8')
    subprocess.run(['cmd.exe', '/d', '/c', str(work / 'build.cmd')], cwd=work, check=True)
    subprocess.run([str(work / 'checks.exe')], cwd=work, check=True)
