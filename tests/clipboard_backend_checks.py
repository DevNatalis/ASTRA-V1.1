"""Test the compiled Win32 ImGui clipboard reader with isolated API mocks."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/security-audit/clipboard'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'src/Includes/ImGui/Files/imgui.cpp').read_text(encoding='utf-8-sig')
start = source.index('static const char* GetClipboardTextFn_DefaultImpl(void* user_data_ctx)\n{')
end = source.index('\nstatic void SetClipboardTextFn_DefaultImpl', start)
reader = source[start:end]
test = r'''
#include <windows.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
struct Buffer {
    std::vector<char> bytes;
    char* Data = nullptr;
    void clear() { bytes.clear(); Data = nullptr; }
    void resize(int n) { bytes.resize(n); Data = bytes.data(); }
    bool empty() const { return bytes.empty(); }
};
struct ImGuiContext { Buffer ClipboardHandlerData; };
WCHAR text[8];
SIZE_T testCapacity;
bool openOk, dataOk, lockOk, convertOk;
int closes, unlocks, locks;
BOOL MockOpenClipboard(HWND) { return openOk; }
BOOL MockCloseClipboard() { ++closes; return TRUE; }
HANDLE MockGetClipboardData(UINT) { return dataOk ? text : nullptr; }
SIZE_T MockGlobalSize(HANDLE) { return testCapacity; }
void* MockGlobalLock(HANDLE) { ++locks; return lockOk ? text : nullptr; }
BOOL MockGlobalUnlock(HANDLE) { ++unlocks; return TRUE; }
int MockConvert(UINT, DWORD, LPCWCH data, int count, LPSTR target, int size, LPCCH, LPBOOL) {
    assert(count > 0 && count <= 8);
    assert(data[count-1] == 0);
    if (!convertOk) return 0;
    if (target) { assert(size >= count); for (int i=0; i<count; ++i) target[i] = static_cast<char>(data[i]); }
    return count;
}
#define OpenClipboard MockOpenClipboard
#define CloseClipboard MockCloseClipboard
#define GetClipboardData MockGetClipboardData
#define GlobalSize MockGlobalSize
#define GlobalLock MockGlobalLock
#define GlobalUnlock MockGlobalUnlock
#define WideCharToMultiByte MockConvert
READER
int main() {
    for (int scenario=0; scenario<9; ++scenario) {
        ImGuiContext context;
        std::memset(text, 0, sizeof(text)); text[0] = L'x';
        testCapacity=sizeof(text); closes=unlocks=locks=0;
        openOk=scenario!=0; dataOk=scenario!=1; lockOk=scenario!=2; convertOk=scenario!=3;
        if (scenario==4) testCapacity=0;
        if (scenario==5) testCapacity=1024*1024+2;
        if (scenario==6) testCapacity=3;
        if (scenario==7) for (auto& c:text) c=L'x';
        const char* result=GetClipboardTextFn_DefaultImpl(&context);
        assert((result!=nullptr)==(scenario==8));
        if(result) assert(std::strcmp(result,"x")==0);
        assert(closes==(openOk?1:0));
        assert(unlocks==((scenario==3||scenario==7||scenario==8)?1:0));
        if (scenario>=4 && scenario<=6) assert(locks==0);
    }
    std::cout << "9 Unicode clipboard boundary and cleanup cases passed\n";
}
'''.replace('READER', reader)
(out / 'checks.cpp').write_text(test, encoding='utf-8')
vcvars = r'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
(out / 'checks.cmd').write_text(f'@call "{vcvars}" >nul\n@cl /nologo /std:c++17 /EHsc /W4 /RTC1 checks.cpp /Fe:checks.exe\n@if errorlevel 1 exit /b 1\n@checks.exe\n@exit /b %errorlevel%\n', encoding='utf-8')
result = subprocess.run(['cmd.exe', '/d', '/c', str(out / 'checks.cmd')], cwd=out, capture_output=True)
(out / 'checks.log').write_bytes(result.stdout + result.stderr)
print((result.stdout + result.stderr).decode('utf-8', errors='replace'))
raise SystemExit(result.returncode)
