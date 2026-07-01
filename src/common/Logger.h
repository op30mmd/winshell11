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

private:
    static std::wstring GetLogPath();
};

#define LOG_DEBUG(msg) shell::common::Logger::Log(shell::common::LogLevel::Debug, msg)
#define LOG_INFO(msg) shell::common::Logger::Log(shell::common::LogLevel::Info, msg)
#define LOG_WARNING(msg) shell::common::Logger::Log(shell::common::LogLevel::Warning, msg)
#define LOG_ERROR(msg) shell::common::Logger::Log(shell::common::LogLevel::Error, msg)

} // namespace shell::common
