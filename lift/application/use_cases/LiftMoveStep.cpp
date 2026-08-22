#include "../lift/application/use_cases/LiftMoveStep.hpp"
#include "../common/ports/IRobotStep.hpp"
#include "LiftEvent.hpp"
#include <future>
#include <variant>

namespace lift::application::use_cases {
    LiftMoveStep::LiftMoveStep(std::shared_ptr<lift::application::adapter::LiftController> lift_controller, lift::domain::value_objects::LiftTarget target,int action_index)
    : common::ports::IRobotStep(action_index),
    liftController_(lift_controller),
    target_(target)
    {

    }
    common::ports::RobotStepResult LiftMoveStep::excute(common::ports::RobotStepResult prevResult)
    {
        auto promise  = std::make_shared<std::promise<common::ports::LiftMoveStepResult>>();
        auto future   = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>(false);

        auto handleId = liftController_->subscribeEvents([&promise,resolved](const lift::domain::events::LiftEvent& event){
            std::visit([&promise,&resolved](const auto& ev){
                using T = std::decay_t<decltype(ev)>;
                if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCompletedEvent>) 
                {
                    if (resolved->exchange(true)) 
                        return;
                    promise->set_value(common::ports::LiftMoveStepResult{.result = common::ports::LiftMoveStepResult::Result::Success});
                }
                else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCanceledEvent>) 
                {
                    if (resolved->exchange(true)) 
                        return;
                    promise->set_value(common::ports::LiftMoveStepResult{.result = common::ports::LiftMoveStepResult::Result::Canceled});
                }
            },event);
        });
        auto ec = liftController_->liftMove(target_);
        if(ec)
        {
            return common::ports::LiftMoveStepResult{.result = common::ports::LiftMoveStepResult::Result::Failed};
        }
        auto result = future.get();
        liftController_->unSubscribeEvents(handleId);
        return result;
    }
    int LiftMoveStep::getActionIndex()
    {
        return actionIndex_;
    }
    
    void LiftMoveStep::cancel()
    {
        liftController_->cancel();
    }

    void LiftMoveStep::pause()
    {
        liftController_->pause();
    }

    void LiftMoveStep::resume()
    {
        liftController_->resume();
    }
}