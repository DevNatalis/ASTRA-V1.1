#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "reg.h"

void clearDllRegistryLogs() {
    HKEY hKey;
    const wchar_t* regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU\\dll";

    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, regPath, 0, KEY_SET_VALUE | KEY_WRITE | KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        if (result == ERROR_FILE_NOT_FOUND) {
            std::wcout << L"[!] Registry path not found. Nothing to clean.\n";
        }
        else {
            std::wcout << L"[!] Error accessing registry: " << result << L"\n";
        }
        return;
    }

    DWORD index = 0;
    DWORD valueNameSize;
    wchar_t valueName[256];
    std::vector<std::wstring> toDelete;

    // Enumerar os valores
    while (true) {
        valueNameSize = sizeof(valueName) / sizeof(wchar_t);
        DWORD type;
        BYTE data[1024];
        DWORD dataSize = sizeof(data);

        result = RegEnumValueW(hKey, index, valueName, &valueNameSize, NULL, &type, data, &dataSize);
        if (result == ERROR_NO_MORE_ITEMS) break;

        if (result == ERROR_SUCCESS) {
            if (wcscmp(valueName, L"MRUListEx") != 0) {
                toDelete.push_back(valueName);
            }
            index++;
        }
        else {
            std::wcout << L"[!] Failed to enumerate values. Error: " << result << L"\n";
            break;
        }
    }

    // Deletar os valores
    for (const auto& name : toDelete) {
        result = RegDeleteValueW(hKey, name.c_str());
        if (result == ERROR_SUCCESS) {
            std::wcout << L"[+] Deleted: " << name << L"\n";
        }
        else {
            std::wcout << L"[!] Failed to delete " << name << L": " << result << L"\n";
        }
    }

    RegCloseKey(hKey);
    std::wcout << L"[*] Registry DLL history cleared.\n";
}