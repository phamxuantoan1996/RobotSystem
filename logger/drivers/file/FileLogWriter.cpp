#include "../logger/drivers/file/FileLogWriter.hpp"

namespace logger::drivers::file {

    void FileLogWriter::write(const logger::domain::entities::LogEntry& entry)
    {
        std::lock_guard<std::mutex> lk(mutex_);

        // Kiểm tra ngày mới — tạo file mới nếu sang ngày
        auto today = currentDateStr();
        if (today != currentDate_)
            openFileForToday();

        if (file_.is_open())
            file_ << format(entry) << std::endl;
    }

    void FileLogWriter::openFileForToday()
    {
        if (file_.is_open())
            file_.close();

        currentDate_ = currentDateStr();
        std::string path = logDir_ + "/robot_" + currentDate_ + ".log";
        file_.open(path, std::ios::app);  // append — tránh mất log nếu restart
    }

    std::string FileLogWriter::currentDateStr() const
    {
        auto now  = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&time, &tm);

        char buf[16];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
        return std::string(buf);
    }

    std::string FileLogWriter::format(const logger::domain::entities::LogEntry& entry) const
    {
        auto time = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::tm tm{};
        localtime_r(&time, &tm);

        char timeBuf[32];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);

        std::string level;
        switch (entry.level) {
            case logger::domain::entities::LogLevel::Info:    level = "INFO "; break;
            case logger::domain::entities::LogLevel::Warning: level = "WARN "; break;
            case logger::domain::entities::LogLevel::Error:   level = "ERROR"; break;
            case logger::domain::entities::LogLevel::Debug:   level = "DEBUG"; break;
        }

        return std::string(timeBuf)
             + " [" + level + "] "
             + "[" + entry.subsystem + "] "
             + entry.message;
    }
}