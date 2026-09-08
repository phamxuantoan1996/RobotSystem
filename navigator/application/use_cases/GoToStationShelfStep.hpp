#pragma once

#include "../common/ports/IRobotStep.hpp"
#include "../navigator/domain/value_objects/station.hpp"
#include "../navigator/application/adapter/NavigatorController.hpp"
#include <memory>
#include <string>
namespace navigator::application::use_cases {
    class GoToStationShelfStep : public common::ports::IRobotStep {
        public:
            GoToStationShelfStep(std::shared_ptr<navigator::application::adapter::NavigatorController> controller,navigator::domain::value_objects::Station station,const std::string& shelf_name,int action_index);
            
            common::ports::RobotStepResult excute(common::ports::RobotStepResult prevResult) override;
            int getActionIndex() override;
            void pause() override;
            void resume() override;
            void cancel() override;
        
        private:
            std::shared_ptr<navigator::application::adapter::NavigatorController> controller_;
            domain::value_objects::Station station_;
            std::string shelfName_;
    };
}