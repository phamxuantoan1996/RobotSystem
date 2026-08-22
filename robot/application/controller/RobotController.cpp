#include "../robot/application/controller/RobotController.hpp"
#include "../common/application/EventBus.hpp"
#include "../board/domain/events/BoardEvent.hpp"
#include "MissionParser.hpp"
#include "MissionStatus.hpp"
#include "Orchestrator.hpp"
#include "RobotEvent.hpp"
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <variant>

namespace robot::application {
    RobotController::RobotController(std::shared_ptr<board::application::adapter::BoardController> boardController,
        std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController,
        std::shared_ptr<gateway::application::adapter::GatewayController> gatewayController,
        std::shared_ptr<lift::application::adapter::LiftController> liftController)
    :boardController_(boardController),
    navigatorController_(navigatorController),
    gatewayController_(std::move(gatewayController)),
    liftController_(liftController),
    robotEventBus_(std::make_unique<common::application::EventBus<robot::domain::events::RobotEvent>>()),
    missionParser_(std::make_unique<robot::domain::value_objects::MissionParser>(navigatorController_,liftController_)),
    orchestrator_(std::make_unique<robot::application::Orchestrator>())
    {
        // dang ki cac event
        //1) gateway event
        gatewayController_->subcribeEvents([this](const gateway::domain::events::GatewayEvent& event){
            std::visit([this](const auto& e) {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, gateway::domain::events::MissionDispatchEvent>)
                {     
                    // std::cout << "mission dispatch\n";
                    auto robot_task = missionParser_->parser(e.mission);
                    if(robot_task)
                    {
                        // std::cout << "Da nhan mission.\n";
                        orchestrator_->enqueueMission(std::move(robot_task.value()));
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalCancelEvent>)
                {   
                    std::cout << "signal cancel.\n";
                    if(orchestrator_->getStepIndex() != -1)
                    {
                        orchestrator_->cancel();
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalPauseEvent>)
                {
                    std::cout << "signal pause.\n";
                    if(orchestrator_->getStepIndex() != -1)
                    {
                        orchestrator_->pause();
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalResumeEvent>)
                {     
                    std::cout << "signal resume.\n";
                    if(orchestrator_->getStepIndex() != -1)
                    {
                        orchestrator_->resume();
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalSwitchModeEvent>)
                {     
                    std::cout << "signal switch mode.\n";
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalClearErrorEvent>)
                {     
                    std::cout << "signal clear error.\n";
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.mission_error = false;
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::ControlManualEvent>)
                {     
                    std::cout << "control manual.\n";
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalCollisionEvent>)
                {     
                    std::cout << "signal collision.\n";
                }
            },event);
        });

        navigatorController_->subscribeEvents([this](const navigator::domain::events::NavigatorEvent& event){
            std::visit([this](const auto& e){
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskStartedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_task_running = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorArrivedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_task_running = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskCanceledEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_task_running = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskSetFailedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_task_running = false;
                        systemError_.navigator_failed = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskClearFailedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_failed = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetErrorEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_task_running = false;
                        systemError_.navigator_error = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearErrorEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_error = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetFatalEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_task_running = false;
                        systemError_.navigator_fatal = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearFatalEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_fatal = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetBlockEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_blocked = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearBlockEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_blocked = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetEmergencyEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_emergency = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearEmergencyEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_emergency = false;
                    }
                    updateRobotStatus();
                }
            }, event);
        });

        liftController_->subscribeEvents([this](const lift::domain::events::LiftEvent& event){
            std::visit([this](const auto& e){
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskRunningEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.lift_task_running = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCanceledEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.lift_task_running = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCompletedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.lift_task_running = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusSetErrorEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.lift_task_running = false;
                        systemError_.lift_error = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusClearErrorEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.lift_error = false;
                    }
                    updateRobotStatus();
                }
            }, event);
        });

        boardController_->subscribeEvents([this](const board::domain::events::BoardEvent& event){
            std::visit([this](const auto& e){
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, board::domain::events::BoardReconnectedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.board_error = false;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, board::domain::events::BoardDisconnectedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.board_error = true;
                    }
                    updateRobotStatus();
                }
            }, event);
        });
    }
    RobotController::~RobotController()
    {
        stop();
    }

    RobotController::HandlerId RobotController::subscribeEvents(RobotController::RobotEventHandler handler)
    {
        return robotEventBus_->subscribe(std::move(handler));
    }
    void RobotController::unSubscribeEvents(RobotController::HandlerId id)
    {
        robotEventBus_->unsubscribe(id);
    }

    void RobotController::start()
    {
        if(running_)
        {
            return;
        }
        running_ = true;
        orchestrator_->setMissionRunningCallback([this](const std::string& mission_id){
            std::cout << "Mission running : " << mission_id << std::endl;
            robotEventBus_->publish(robot::domain::events::MissionRunningEvent{.mission_id = mission_id});
            {
                std::lock_guard<std::mutex> lk(mutexState_);
                systemError_.mission_running = true;
                missionStatus_ = robot::domain::entities::MissionStatusCode::Cargo;
            }
            updateRobotStatus();
        });
        orchestrator_->setMissionCompletedCallback([this](const std::string& mission_id){
            std::cout << "Mission completed : " << mission_id << std::endl;
            robotEventBus_->publish(robot::domain::events::MissionCompletedEvent{.mission_id = mission_id});
            {
                std::lock_guard<std::mutex> lk(mutexState_);
                systemError_.mission_running = false;
                missionStatus_ = robot::domain::entities::MissionStatusCode::Completed;
            }
            updateRobotStatus();
        });
        orchestrator_->setMissionCanceledCallback([this](const std::string& mission_id){
            std::cout << "Mission canceled : " << mission_id << std::endl;
            robotEventBus_->publish(robot::domain::events::MissionCanceledEvent{.mission_id = mission_id});
            {
                std::lock_guard<std::mutex> lk(mutexState_);
                systemError_.mission_running = false;
                missionStatus_ = robot::domain::entities::MissionStatusCode::Cancel;
            }
            updateRobotStatus();
        });
        orchestrator_->setMissionErrorCallback([this](const std::string& mission_id){
            std::cout << "Mission error : " << mission_id << std::endl;
            robotEventBus_->publish(robot::domain::events::MissionErrorEvent{.mission_id = mission_id});
            {
                std::lock_guard<std::mutex> lk(mutexState_);
                systemError_.mission_running = false;
                systemError_.mission_error = true;
                missionStatus_ = robot::domain::entities::MissionStatusCode::Error;
            }
            updateRobotStatus();
        });

        gatewayController_->setGetRobotStatusCallback([this](void){

            /*
            {
                "navigator" : {}
                "lift" : {
                    "lift_position" : -1,
                    "status" : 0,
                    "error_code" : []
                }
                "robot_status" : 0,
                "mission" : {
                    "mission_code" : "",
                    "step" : -1,
                    "mission_status" : 0
                }
            }
            */
            auto liftState = liftController_->getState();
            auto navigatorState = navigatorController_->state();
            
            Json::Value root;

            Json::Value mission_state;
            mission_state["mission_code"] = orchestrator_->getMissionId();
            mission_state["step"] = orchestrator_->getStepIndex();
            mission_state["mission_status"] = static_cast<int>(missionStatus_);
            root["mission"] = mission_state;

            Json::Value lift_state;
            lift_state["lift_position"] = liftState.lift_position;
            lift_state["lift_device_status"] = static_cast<int>(liftState.device_status);
            lift_state["lift_task_status"] = static_cast<int>(liftState.task_status);
            Json::Value lift_errors(Json::arrayValue);
            for(const auto& error : liftState.error_codes)
            {
                if (error != 0) {
                    lift_errors.append(error);
                }
            }
            lift_state["lift_error"] = lift_errors;
            root["lift"] = lift_state;


            Json::Value stateObj;
            Json::CharReaderBuilder builder_navigator;
            std::string errs;
            std::istringstream iss(navigatorState.state_raw);
            if (Json::parseFromStream(builder_navigator, iss, &stateObj, &errs)) {
                root["navigator"] = stateObj;
            } else {
                std::cerr << "Failed to parse state_raw: " << errs << std::endl;
                root["navigator"] = Json::Value(Json::nullValue);
            }

            Json::StreamWriterBuilder builder;
            builder["indentation"] = ""; 
            std::string status = Json::writeString(builder, root);
            return status;
        });
        orchestrator_->start();
        workerThread = std::thread(&RobotController::workerLoop,this);
        
    }
    void RobotController::stop()
    {
        running_ = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
    void RobotController::workerLoop()
    {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    void RobotController::updateRobotStatus(void)
    {
        std::lock_guard<std::mutex> lk(mutexState_);
        if(systemError_.navigator_emergency || systemError_.navigator_error || systemError_.navigator_failed 
            || systemError_.navigator_fatal || systemError_.navigator_disconnected || systemError_.lift_error
            || systemError_.mission_error || systemError_.board_error)
        {
            robotStatus_ = robot::domain::entities::RobotStatusCode::Error;
        }
        else if(systemError_.navigator_blocked)
        {
            robotStatus_ = robot::domain::entities::RobotStatusCode::Stop;
        }
        else if(systemError_.pause_by_manual)
        {
            robotStatus_ = robot::domain::entities::RobotStatusCode::PauseManual;
        }
        else if(systemError_.mission_running || systemError_.lift_task_running || systemError_.navigator_task_running)
        {
            robotStatus_ = robot::domain::entities::RobotStatusCode::Active;
        }
        else
        {
            robotStatus_ = robot::domain::entities::RobotStatusCode::Idle;
        }
        std::cout << "update robot status : " << static_cast<int>(robotStatus_) << std::endl;
    }
}