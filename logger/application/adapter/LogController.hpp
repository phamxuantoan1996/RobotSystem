// LogSubsystem/adapters/LogController.hpp
#pragma once
#include "../logger/ports/ILogWriter.hpp"
#include "../logger/domain/entities/LogEntry.hpp"
#include "../navigator/application/adapter/NavigatorController.hpp"
#include "../board/application/adapter/BoardController.hpp"
#include "../lift/application/adapter/LiftController.hpp"
#include "../robot/application/controller/RobotController.hpp"
#include <memory>
#include <string>

namespace logger::application::adapter {

    class LogController {
        public:
            LogController(
                std::unique_ptr<logger::ports::ILogWriter> writer,
                std::shared_ptr<robot::application::RobotController> robot,
                std::shared_ptr<board::application::adapter::BoardController> board,
                std::shared_ptr<navigator::application::adapter::NavigatorController> navigator,
                std::shared_ptr<lift::application::adapter::LiftController> lift);

        private:
            using LogLevel = logger::domain::entities::LogLevel;

            void log(logger::domain::entities::LogLevel level,
                    const std::string& subsystem,
                    const std::string& message);

            std::shared_ptr< logger::ports::ILogWriter> writer_;
    };

}