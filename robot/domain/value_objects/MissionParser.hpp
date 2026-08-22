#pragma once
#include "../robot/domain/entities/RobotTask.hpp"
#include "../navigator/application/adapter/NavigatorController.hpp"
#include "../lift/application/adapter/LiftController.hpp"
#include <memory>
#include <optional>
#include <string>

namespace robot::domain::value_objects {
    class MissionParser {
        public:
            MissionParser(std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController,
                std::shared_ptr<lift::application::adapter::LiftController> liftController);
            std::optional<robot::domain::entities::RobotTask> parser(std::string mission_raw);

        private:
            std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController_;
            std::shared_ptr<lift::application::adapter::LiftController> liftController_;
    };
}