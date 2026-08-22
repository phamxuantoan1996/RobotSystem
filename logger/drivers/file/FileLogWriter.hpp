// LogSubsystem/infrastructure/FileLogWriter.hpp
#pragma once
#include "../logger/ports/ILogWriter.hpp"
#include <fstream>
#include <filesystem>
#include <mutex>
#include <string>

namespace logger::drivers::file {

class FileLogWriter : public logger::ports::ILogWriter {
public:
    explicit FileLogWriter(const std::string& logDir = "./logs")
        : logDir_(logDir)
    {
        std::filesystem::create_directories(logDir_);
        openFileForToday();
    }

    void write(const logger::domain::entities::LogEntry& entry) override;

private:
    void openFileForToday();

    std::string currentDateStr() const;

    std::string format(const logger::domain::entities::LogEntry& entry) const;

    std::string   logDir_;
    std::string   currentDate_;
    std::ofstream file_;
    std::mutex    mutex_;
};

}