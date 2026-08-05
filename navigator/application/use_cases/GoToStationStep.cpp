#include "GoToStationStep.hpp"
#include "../common/ports/IRobotStep.hpp"
#include "NavigatorEvent.hpp"
#include <atomic>
#include <future>

namespace navigator::application::use_cases {
    GoToStationStep::GoToStationStep(std::shared_ptr<navigator::application::adapter::NavigatorController> controller,std::string station) 
    : controller_(controller)
    ,station_(station)
    {

    }
    common::ports::RobotStepResult GoToStationStep::excute(common::ports::RobotStepResult prevResult)
    {
        auto promise  = std::make_shared<std::promise<common::ports::GotoStationStepResult>>();
        auto future   = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>(false);
        auto handleId = controller_->subcribeEvents([&promise,resolved](const navigator::domain::events::NavigatorEvent& event){
            std::visit([&promise,&resolved](const auto& ev)
            {
                using T = std::decay_t<decltype(ev)>;
                if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorArrivedEvent>) 
                {
                    if (resolved->exchange(true)) 
                        return;
                    promise->set_value(common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Success});
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskCanceledEvent>) {
                    if (resolved->exchange(true)) return;
                    promise->set_value(common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Canceled});
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetErrorEvent>) {
                    if (resolved->exchange(true)) return;
                    promise->set_value(common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Failed});
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetFatalEvent>) {
                    if (resolved->exchange(true)) return;
                    promise->set_value(common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Failed});
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskSetFailedEvent>) {
                    if (resolved->exchange(true)) return;
                    promise->set_value(common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Failed});
                }
            },event);
        });
        auto ec = controller_->goToStation(domain::value_objects::Station(station_));
        if(ec)
        {
            return common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Failed};
        }
        auto result = future.get();
        controller_->unSubcribeEvents(handleId);
        return result;
    }
}