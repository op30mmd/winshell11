#include "Logger.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <shlobj.h>
#include <windows.h>

namespace shell::common {

static std::string g_processName;

void Logger::Initialize(std::string_view processName) {
    g_processName = processName;
}

void Logger::Log(LogLevel /*level*/, std::wstring_view message) {
    std::wcout << L"[" << message << L"]" << std::endl;
}

void Logger::Log(LogLevel /*level*/, std::string_view message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    struct tm buf;
    localtime_s(&buf, &in_time_t);
    std::cout << std::put_time(&buf, "%Y-%m-%d %X") << " [" << g_processName << "] " << message << std::endl;
}

std::wstring Logger::GetLogPath() {
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &path))) {
        std::wstring logPath = path;
        logPath += L"\\CustomShell\\logs\\";
        CoTaskMemFree(path);
        return logPath;
    }
    return L"";
}

} // namespace shell::common
