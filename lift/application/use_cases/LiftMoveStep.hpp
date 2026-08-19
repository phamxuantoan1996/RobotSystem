#pragma once
#include "../lift/application/adapter/LiftController.hpp"
#include "../lift/domain/value_objects/LiftTarget.hpp"
#include "../common/ports/IRobotStep.hpp"
#include <memory>

namespace lift::application::use_cases {
    class LiftMoveStep : public common::ports::IRobotStep {
        public:
            LiftMoveStep(std::shared_ptr<lift::application::adapter::LiftController> lift_controller, lift::domain::value_objects::LiftTarget target,int action_index);
            common::ports::RobotStepResult excute(common::ports::RobotStepResult prevResult) override;
            int getActionIndex() override;
        private:
            std::shared_ptr<lift::application::adapter::LiftController> liftController_;
            lift::domain::value_objects::LiftTarget target_;
    };
}