#pragma once
#include "../common/ports/IRobotStep.hpp"
#include <cstdint>

namespace navigator::application::use_cases {
    class WaitStep : public common::ports::IRobotStep {
        public:
            WaitStep(uint32_t wait_time,int action_index);
            common::ports::RobotStepResult excute(common::ports::RobotStepResult prevResult) override;
            int getActionIndex() override;
            void pause() override;
            void resume() override;
            void cancel() override;
        private:
            uint32_t waitTime_ = 0;
    };
}
