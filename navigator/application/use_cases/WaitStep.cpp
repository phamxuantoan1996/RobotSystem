#include "../navigator/application/use_cases/WaitStep.hpp"
#include "../common/ports/IRobotStep.hpp"
#include <cstdint>
#include <thread>

namespace navigator::application::use_cases {
    WaitStep::WaitStep(uint32_t wait_time,int action_index) : common::ports::IRobotStep(action_index), waitTime_(wait_time)
    {

    }
    common::ports::RobotStepResult WaitStep::excute(common::ports::RobotStepResult prevResult)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime_));
        return common::ports::UnknowStepResult{};
    }
    int WaitStep::getActionIndex()
    {
        return actionIndex_;
    }
    void WaitStep::pause()
    {

    }
    void WaitStep::resume()
    {

    }
    void WaitStep::cancel()
    {

    }
}