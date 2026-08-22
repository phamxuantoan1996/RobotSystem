#include "../reactor/indicator/IndicatorReactor.hpp"
#include "../indicator/domain/entities/ColorType.hpp"
#include <iostream>
#include <thread>
#include <variant>

namespace reactor {
    
    IndicatorReactor::IndicatorReactor(std::shared_ptr<robot::application::RobotController> robot_controller,
                std::shared_ptr<board::application::adapter::BoardController> board_controller,
                std::shared_ptr<navigator::application::adapter::NavigatorController> navigator_controller,
                std::unique_ptr<indicator::application::adapter::IndicatorController> indicator_controller,
                std::shared_ptr<lift::application::adapter::LiftController> lift_controller)
    :indicatorController_(std::move(indicator_controller))
    {
        robot_controller->subscribeEvents([this](const robot::domain::events::RobotEvent& event){
            onRobotEvent(event);
        });
        board_controller->subscribeEvents([this](const board::domain::events::BoardEvent& event){
            onBoardEvent(event);
        });
        navigator_controller->subscribeEvents([this](const navigator::domain::events::NavigatorEvent & event){
            onNavigatorEvent(event);
        });
        lift_controller->subscribeEvents([this](const lift::domain::events::LiftEvent& event){
            onLiftEvent(event);
        });
    }

    void IndicatorReactor::onNavigatorEvent(const navigator::domain::events::NavigatorEvent& e)
    {
        std::visit([this](const auto& ev)
        {
            using T = std::decay_t<decltype(ev)>;
            // set light
            if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetEmergencyEvent>)
            {
                {// tranh lock self mutex
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_emergency    = true;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearEmergencyEvent>)
            {
                {// tranh lock self mutex
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_emergency    = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorDisconnectEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    // indicatorSystemState_.nav_disconnected = true;
                    indicatorReactorState_.navigator_disconnected = true;
                }
                updateLight();
            }
                
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorReconnectEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    // indicatorSystemState_.nav_disconnected = false;
                    indicatorReactorState_.navigator_disconnected = false;
                }
                updateLight();
            }
                
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetBlockEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_blocked = true;
                }
                updateLight();
            }
                
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearBlockEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_blocked = false;
                }
                
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskSetFailedEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_task_running = false;
                    indicatorReactorState_.navigator_failed   = true;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskClearFailedEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_failed   = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetErrorEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_task_running = false;
                    indicatorReactorState_.navigator_error = true;
                }
                
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearErrorEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_error = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetFatalEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_task_running = false;
                    indicatorReactorState_.navigator_fatal = true;
                }
                
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearFatalEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_fatal = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskStartedEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_task_running = true;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorArrivedEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_task_running = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskCanceledEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.navigator_task_running = false;
                }
                updateLight();
            }
            
        },e);
    }
    void IndicatorReactor::onRobotEvent(const robot::domain::events::RobotEvent& e)
    {
        std::visit([this](const auto& event){
            using T = std::decay_t<decltype(event)>;
            if constexpr (std::is_same_v<T, robot::domain::events::MissionRunningEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.mission_running = true;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, robot::domain::events::MissionCompletedEvent>){
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.mission_running = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, robot::domain::events::MissionCanceledEvent>){
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.mission_running = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, robot::domain::events::MissionErrorEvent>){
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.mission_running = false;
                }
                updateLight();
            }
        },e);
    }
    void IndicatorReactor::onBoardEvent(const board::domain::events::BoardEvent& e)
    {
        std::visit([this](const auto& event){
            using T = std::decay_t<decltype(event)>;
            if constexpr (std::is_same_v<T, board::domain::events::BoardDisconnectedEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.board_error = true;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, board::domain::events::BoardReconnectedEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.board_error = false;
                }
                updateLight();
            }
        },e);
    }
    void IndicatorReactor::onLiftEvent(const lift::domain::events::LiftEvent& e)
    {
        std::visit([this](const auto& event){
            using T = std::decay_t<decltype(event)>;
            if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusSetErrorEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.lift_error = true;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusClearErrorEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.lift_error = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskRunningEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.lift_task_running = true;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCompletedEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.lift_task_running = false;
                }
                updateLight();
            }
            else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCanceledEvent>)
            {
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    indicatorReactorState_.lift_task_running = false;
                }
                updateLight();
            }
        },e);
    }

    void IndicatorReactor::updateLight(void)
    {
        IndicatorReactorState stateSnapshot;
        {
            std::lock_guard<std::mutex> lk(mutexState_);
            stateSnapshot = indicatorReactorState_;
        }
        indicator::domain::entities::ColorType newColor = indicator::domain::entities::ColorType::Off;

        if(stateSnapshot.navigator_emergency)
        {
            newColor = indicator::domain::entities::ColorType::RedBlink;
        }
        else if(stateSnapshot.lift_error || stateSnapshot.navigator_blocked || stateSnapshot.navigator_disconnected || stateSnapshot.navigator_failed || stateSnapshot.navigator_fatal || stateSnapshot.navigator_error || stateSnapshot.board_error)
        {
            newColor = indicator::domain::entities::ColorType::Red;
        }
        else if (stateSnapshot.navigator_task_running || stateSnapshot.lift_task_running || stateSnapshot.mission_running)
        {
            /* code */
            newColor = indicator::domain::entities::ColorType::Green;
        }
        else
        {
            newColor = indicator::domain::entities::ColorType::Yellow;
        }

        if(newColor == currentColor_)
        {
            return;
        }

        switch(newColor)
        {
            case indicator::domain::entities::ColorType::RedBlink:
            {
                std::cout << "blink red led\n";
                break;
            }
            case indicator::domain::entities::ColorType::Red:
            {
                 std::cout << "red led\n";
                break;
            }
            case indicator::domain::entities::ColorType::Green:
            {
                std::cout << "green led\n";
                break;
            }
            case indicator::domain::entities::ColorType::Yellow:
            {
                std::cout << "yellow led\n";
                break;
            }
            case indicator::domain::entities::ColorType::GreenBlink:
            {
                std::cout << "blink green led\n";
                break;
            }
            case indicator::domain::entities::ColorType::Off:
            {
                std::cout << "off led\n";
                break;
            }
            default:
                break;
        }
        currentColor_ = newColor;
        {
            std::lock_guard<std::mutex> lock(mutexQueue_);
            colorQueue_.push(std::move(currentColor_));
        }
        cvQueue_.notify_one(); 
    }

    IndicatorReactor::~IndicatorReactor()
    {
        running_ = false;
        cvQueue_.notify_all(); 
        if(workerThread_.joinable())
        {
            workerThread_.join();
        }
    }

    void IndicatorReactor::start(void)
    {
        if (running_) {
            return;
        }
        running_ = true;
        workerThread_ = std::thread(&IndicatorReactor::workerLoop,this);
    }

    void IndicatorReactor::workerLoop(void)
    {
        while (running_) 
        {
            indicator::domain::entities::ColorType color;
            {
                std::unique_lock<std::mutex> lock(mutexQueue_);
                cvQueue_.wait(lock, [this]() { 
                    return ! colorQueue_.empty() || !running_; 
                });

                if (!running_) 
                {
                    break;
                }

                if(colorQueue_.empty())
                {
                    continue;
                }

                color = std::move(colorQueue_.front());
                colorQueue_.pop();
            }
            auto ec = indicatorController_->setColor(color);
            if(ec)
            {
                
            }
        }
    }
}