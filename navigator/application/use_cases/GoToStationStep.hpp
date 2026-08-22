#pragma once
#include "../common/ports/IRobotStep.hpp"
#include "../navigator/application/adapter/NavigatorController.hpp"
#include "../navigator/domain/value_objects/station.hpp"
#include <memory>
namespace navigator::application::use_cases {
    class GoToStationStep : public common::ports::IRobotStep {
        public:
            GoToStationStep(std::shared_ptr<navigator::application::adapter::NavigatorController> controller,const domain::value_objects::Station& station,int action_index);
            common::ports::RobotStepResult excute(common::ports::RobotStepResult prevResult) override;
            int getActionIndex() override;
            void pause() override;
            void resume() override;
            void cancel() override;
        private:
            std::shared_ptr<navigator::application::adapter::NavigatorController> controller_;
            domain::value_objects::Station station_;
    };
}
