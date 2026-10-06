"""Run isolated regression checks; optional --live PID reads names only."""
from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / 'build' / 'player-feature-checks'
out.mkdir(parents=True, exist_ok=True)

def method(path, signature):
    text = (root / path).read_text(encoding='utf-8-sig')
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

source = r'''
#include <windows.h>
#include <tlhelp32.h>
#include <cassert>
#include <algorithm>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include "Core/SDK/PlayerNames.hpp"
struct Memory {
    HANDLE ProcHandle = nullptr;
    template<class T> T Read(uintptr_t p) {
        T value{}; SIZE_T received = 0;
        ReadProcessMemory(ProcHandle, (LPCVOID)p, &value, sizeof(value), &received);
        return value;
    }
    uintptr_t GetModuleBaseAddr(DWORD pid, const char* name, uintptr_t* size) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE) { std::cerr << "Snapshot failed: " << GetLastError() << "\n"; return 0; }
        MODULEENTRY32 entry{}; entry.dwSize = sizeof(entry);
        uintptr_t result = 0;
        if (Module32First(snapshot, &entry)) do {
            if (_stricmp(name, entry.szModule) == 0) {
                result = (uintptr_t)entry.modBaseAddr; *size = entry.modBaseSize; break;
            }
        } while (Module32Next(snapshot, &entry));
        if (!result) std::cerr << "Module missing: " << name << "; last error " << GetLastError() << "\n";
        CloseHandle(snapshot); return result;
    }
} Mem;
struct { DWORD ProcIdFiveM = 0; } g_Variables;
struct { int CurrentBuild = 3258; uintptr_t m_PlayerId = 0x7C; } g_Offsets;
namespace Debug { void Warning(const char*, const char*, unsigned) {} }
struct CPlayerInfo { PLAYER_ID };
struct CPed {
    CPlayerInfo* GetPlayerInfo() { return Mem.Read<CPlayerInfo*>((uintptr_t)this + 0x10A8); }
    GET_ID
    GOD_MODE
};
struct Entities { GET_NAME };
struct EntityStruct { std::string Name; };
std::vector<EntityStruct> EntityList;
std::shared_mutex EntityListMutex;
GET_SNAPSHOT
PUBLISH_SNAPSHOT
int main(int argc, char** argv) {
    if (argc == 2) {
        g_Variables.ProcIdFiveM = std::stoul(argv[1]);
        Mem.ProcHandle = OpenProcess(PROCESS_VM_READ, FALSE, g_Variables.ProcIdFiveM);
        assert(Mem.ProcHandle);
        uintptr_t size = 0;
        const auto base = Mem.GetModuleBaseAddr(g_Variables.ProcIdFiveM, "FiveM_b3258_GTAProcess.exe", &size);
        assert(base);
        const auto replay = Mem.Read<uintptr_t>(base + 0x1FBD4F0);
        const auto pedInterface = Mem.Read<uintptr_t>(replay + 0x18);
        const auto list = Mem.Read<uintptr_t>(pedInterface + 0x100);
        const int count = (std::min)(512, Mem.Read<int>(pedInterface + 0x108));
        Entities entities;
        int players = 0, matched = 0, longNames = 0;
        for (int i = 0; i < count; ++i) {
            auto ped = Mem.Read<CPed*>(list + i * 16);
            if (!ped || !ped->GetPlayerInfo()) continue;
            ++players;
            auto name = entities.getPlayerNameByNetId(ped->GetID());
            if (!name.empty()) ++matched;
            if (name.size() > 15) ++longNames;
        }
        std::cout << "Live read-only: " << matched << "/" << players
                  << " names resolved; long names: " << longNames << "\n";
        CloseHandle(Mem.ProcHandle);
        return players > 0 && matched == players ? 0 : 1;
    }
    // Names owned by a render snapshot survive replacement/destruction by the worker.
    const std::string stableName(128, 'A');
    std::vector<EntityStruct> firstFrame{{stableName}};
    PublishEntityList(firstFrame);
    const auto frame = GetEntityListSnapshot();
    std::thread writer([] {
        for (int i = 0; i < 2000; ++i) {
            std::vector<EntityStruct> next(32, {std::string(128, char('B' + i % 20))});
            PublishEntityList(next);
        }
    });
    for (int i = 0; i < 2000; ++i) {
        const auto snapshot = GetEntityListSnapshot();
        for (const auto& entity : snapshot) assert(entity.Name == snapshot.front().Name);
        assert(frame.front().Name == stableName);
    }
    writer.join();
    assert(frame.front().Name == stableName);
    Mem.ProcHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE, GetCurrentProcessId());
    assert(Mem.ProcHandle);
    auto reader = [](uintptr_t address, void* buffer, size_t size) {
        SIZE_T received = 0;
        return ReadProcessMemory(Mem.ProcHandle, (LPCVOID)address, buffer, size, &received)
            && received == size;
    };
    std::unordered_map<int, std::string> original, decoded;
    for (int i = 1; i <= 697; ++i) original[i] = "Player " + std::to_string(i);
    original[21] = "Long player name with UTF-8: \xc3\xa9";
    uintptr_t entry = 0;
    for (size_t offset = 0; offset + 16 <= sizeof(original); offset += 8) {
        const auto candidate = (uintptr_t)&original + offset;
        if (PlayerNames::ReadList(reader, candidate, decoded)) { entry = candidate; break; }
    }
    assert(entry && decoded == original);
    assert(!PlayerNames::ReadList(reader, 0, decoded));
    const uintptr_t head = Mem.Read<uintptr_t>(entry);
    const uintptr_t first = Mem.Read<uintptr_t>(head);
    auto cyclic = [&](uintptr_t address, void* buffer, size_t size) {
        if (!reader(address, buffer, size)) return false;
        if (address == first && size == 56) std::memcpy(buffer, &first, 8);
        return true;
    };
    assert(!PlayerNames::ReadList(cyclic, entry, decoded));
    assert(decoded == original); // failed reads cannot publish a partial map
    std::vector<unsigned char> pedMemory(0x1200), infoMemory(0x200);
    auto ped = reinterpret_cast<CPed*>(pedMemory.data());
    auto info = reinterpret_cast<CPlayerInfo*>(infoMemory.data());
    std::memcpy(pedMemory.data()+0x10A8, &info, sizeof(info));
    int expectedId = 8163, wrongId = 31060;
    std::memcpy(infoMemory.data()+0xE8, &expectedId, 4);
    std::memcpy(pedMemory.data()+0xE8, &wrongId, 4);
    assert(ped->GetID() == expectedId);
    DWORD initial = 0x40000C;
    std::memcpy(pedMemory.data()+0x188, &initial, 4);
    assert(ped->SetGodMode(true));
    assert(Mem.Read<DWORD>((uintptr_t)ped+0x188) == (initial | 0x100));
    assert(ped->SetGodMode(false));
    assert(Mem.Read<DWORD>((uintptr_t)ped+0x188) == initial);
    DWORD legacy = initial | 0x200;
    std::memcpy(pedMemory.data()+0x188, &legacy, 4);
    assert(ped->SetGodMode(false));
    assert(Mem.Read<DWORD>((uintptr_t)ped+0x188) == initial);
    CloseHandle(Mem.ProcHandle); Mem.ProcHandle = nullptr;
    assert(!ped->SetGodMode(true));
    std::cout << "Regression checks passed: concurrent snapshots, 697 names, long UTF-8, cycles, IDs, flags, failed writes\n";
}
'''
for placeholder, path, signature in (
    ('PLAYER_ID', 'src/Core/SDK/Structs/GameClasses.hpp', 'int PlayerID()'),
    ('GET_ID', 'src/Core/SDK/Structs/GameClasses.hpp', 'int GetID()'),
    ('GOD_MODE', 'src/Core/SDK/Structs/GameClasses.hpp', 'bool SetGodMode(bool Toggle)'),
    ('GET_NAME', 'src/Core/Threads/EntityList.hpp', 'inline std::string getPlayerNameByNetId'),
    ('GET_SNAPSHOT', 'src/Core/SDK/SDK.hpp', 'inline std::vector<EntityStruct> GetEntityListSnapshot'),
    ('PUBLISH_SNAPSHOT', 'src/Core/SDK/SDK.hpp', 'inline void PublishEntityList'),
):
    source = source.replace(placeholder, method(path, signature))
(out / 'check.cpp').write_text(source, encoding='utf-8')
(out / 'check.vcxproj').write_text('''<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
<ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Debug|x64"><Configuration>Debug</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup>
<PropertyGroup Label="Globals"><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props"/>
<PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><PlatformToolset>v143</PlatformToolset></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props"/>
<ItemDefinitionGroup><ClCompile><LanguageStandard>stdcpp17</LanguageStandard><PreprocessorDefinitions>_ITERATOR_DEBUG_LEVEL=0;%(PreprocessorDefinitions)</PreprocessorDefinitions><AdditionalIncludeDirectories>$(ProjectDir)..\\..\\src;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories></ClCompile></ItemDefinitionGroup>
<ItemGroup><ClCompile Include="check.cpp"/></ItemGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets"/>
</Project>''', encoding='utf-8')
msbuild = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe'
subprocess.run([str(msbuild), str(out / 'check.vcxproj'), '/p:Configuration=Debug', '/p:Platform=x64', '/nologo', '/verbosity:minimal'], check=True)
subprocess.run([str(out / 'x64/Debug/check.exe')], check=True)
if len(sys.argv) == 3 and sys.argv[1] == '--live':
    subprocess.run([str(out / 'x64/Debug/check.exe'), str(int(sys.argv[2]))], check=True)

