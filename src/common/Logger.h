#pragma once

#include <string>
#include <string_view>

namespace shell::common {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static void Initialize(std::string_view processName);
    static void Log(LogLevel level, std::wstring_view message);
    static void Log(LogLevel level, std::string_view message);
    static void LogF(LogLevel level, const char* format, ...);

private:
    static std::wstring GetLogPath();
};

// Printf-style logging (supports format strings with arguments).
// Usage: LOG_INFO("count = %d", count);
// For simple string logging without formatting, just pass a single string.
#define LOG_DEBUG(...) shell::common::Logger::LogF(shell::common::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) shell::common::Logger::LogF(shell::common::LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) shell::common::Logger::LogF(shell::common::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) shell::common::Logger::LogF(shell::common::LogLevel::Error, __VA_ARGS__)

} // namespace shell::common
