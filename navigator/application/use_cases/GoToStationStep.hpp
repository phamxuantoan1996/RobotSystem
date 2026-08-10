#pragma once
#include "../common/ports/IRobotStep.hpp"
#include "../navigator/application/adapter/NavigatorController.hpp"
#include "../navigator/domain/value_objects/station.hpp"
#include <memory>
#include <string>
namespace navigator::application::use_cases {
    class GoToStationStep : public common::ports::IRobotStep {
        public:
            GoToStationStep(std::shared_ptr<navigator::application::adapter::NavigatorController> controller,domain::value_objects::Station station);
            common::ports::RobotStepResult excute(common::ports::RobotStepResult prevResult) override;

        private:
            std::shared_ptr<navigator::application::adapter::NavigatorController> controller_;
            domain::value_objects::Station station_;
    };
}
