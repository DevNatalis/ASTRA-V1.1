#pragma once
#include <string>

namespace TaskDeleter
{
    bool DeleteTask(const std::wstring& folderPath, const std::wstring& taskName);
    void deleteScheduledTask();
}
