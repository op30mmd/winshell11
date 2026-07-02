#include "Logger.h"
#include <chrono>
#include <cstdarg>
#include <cstdio>
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

static void OutputToDebugger(const std::string& message) {
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
}

void Logger::Log(LogLevel /*level*/, std::wstring_view message) {
    std::wstring output = std::wstring(L"[") + std::wstring(message) + L"]";
    std::wcout << output << std::endl;
}

void Logger::Log(LogLevel /*level*/, std::string_view message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    struct tm buf;
    localtime_s(&buf, &in_time_t);

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %X", &buf);

    std::string output = std::string(timestamp) + " [" + g_processName + "] " + std::string(message);
    std::cout << output << std::endl;
    OutputToDebugger(output);
}

void Logger::LogF(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Log(level, std::string_view(buffer));
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
