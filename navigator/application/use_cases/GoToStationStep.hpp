#pragma once
#include "../common/ports/IRobotStep.hpp"
#include "NavigatorController.hpp"
#include <memory>
#include <string>
namespace navigator::application::use_cases {
    class GoToStationStep : common::ports::IRobotStep {
        public:
            GoToStationStep(std::shared_ptr<navigator::application::adapter::NavigatorController> controller,std::string station);
            common::ports::RobotStepResult excute(common::ports::RobotStepResult prevResult) override;

        private:
            std::shared_ptr<navigator::application::adapter::NavigatorController> controller_;
            std::string station_;
    };
}
