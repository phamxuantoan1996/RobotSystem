// LogSubsystem/domain/LogEntry.hpp
#pragma once
#include <string>
#include <chrono>

namespace logger::domain::entities {

    enum class LogLevel {
        Info,
        Warning,
        Error,
        Debug
    };

    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        LogLevel                              level;
        std::string                           subsystem;  // "Nav", "Fork", "Board"...
        std::string                           message;
    };

} // namespace log_subsystem::domain