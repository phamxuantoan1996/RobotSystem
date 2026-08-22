// LogSubsystem/application/ports/ILogWriter.hpp
#pragma once
#include "../logger/domain/entities/LogEntry.hpp"

namespace logger::ports {

class ILogWriter {
public:
    virtual ~ILogWriter() = default;
    virtual void write(const  logger::domain::entities::LogEntry& entry) = 0;
};

} // namespace log_subsystem::application