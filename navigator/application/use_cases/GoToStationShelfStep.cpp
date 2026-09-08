#include "GoToStationShelfStep.hpp"
#include "../common/ports/IRobotStep.hpp"
#include "NavigatorEvent.hpp"
#include "station.hpp"
#include <atomic>
#include <future>

namespace navigator::application::use_cases {
    GoToStationShelfStep::GoToStationShelfStep(std::shared_ptr<navigator::application::adapter::NavigatorController> controller,navigator::domain::value_objects::Station station,const std::string& shelf_name,int action_index)
    : common::ports::IRobotStep(action_index), 
    controller_(controller),
    station_(station),
    shelfName_(shelf_name)
    {
        
    }

    common::ports::RobotStepResult GoToStationShelfStep::excute(common::ports::RobotStepResult prevResult)
    {
        if(auto ec = controller_->setShelf("shelf/" + shelfName_ + ".shelf"))
        {
            return common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Failed};
        }

        auto promise  = std::make_shared<std::promise<common::ports::GotoStationStepResult>>();
        auto future   = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>(false);
        auto handleId = controller_->subscribeEvents([&promise,resolved](const navigator::domain::events::NavigatorEvent& event){
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
        if(auto ec = controller_->goToStation(station_))
        {
            controller_->clearShelf();
            return common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Failed};
        }
        auto result = future.get();
        controller_->unSubscribeEvents(handleId);
        if(auto ec = controller_->clearShelf())
        {
            return common::ports::GotoStationStepResult{.result = common::ports::GotoStationStepResult::Result::Failed};
        }
        return result;
    }

    int GoToStationShelfStep::getActionIndex()
    {
        return actionIndex_;
    }
    void GoToStationShelfStep::pause()
    {
        controller_->pause();
    }
    void GoToStationShelfStep::resume()
    {
        controller_->resume();
    }
    void GoToStationShelfStep::cancel()
    {
        controller_->cancel();
    }


}