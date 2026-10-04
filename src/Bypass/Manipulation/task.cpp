#include "task.hpp"

#include <windows.h>
#include <taskschd.h>
#include <comdef.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "ole32.lib")

namespace TaskDeleter
{
    bool DeleteTask(const std::wstring& folderPath, const std::wstring& taskName)
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr))
            return false;

        ITaskService* pService = nullptr;
        hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr))
        {
            CoUninitialize();
            return false;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr))
        {
            pService->Release();
            CoUninitialize();
            return false;
        }

        ITaskFolder* pFolder = nullptr;
        hr = pService->GetFolder(_bstr_t(folderPath.c_str()), &pFolder);
        if (FAILED(hr))
        {
            pService->Release();
            CoUninitialize();
            return false;
        }

        hr = pFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
        pFolder->Release();
        pService->Release();
        CoUninitialize();

        return SUCCEEDED(hr);
    }

    void deleteScheduledTask()
    {
        const std::wstring folder = L"\\Microsoft\\Windows\\Bluetooth";
        const std::wstring task = L"UninstallDeviceTask";
        DeleteTask(folder, task);
    }
}
