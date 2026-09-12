#include "../robot/application/controller/RobotController.hpp"
#include "../common/application/EventBus.hpp"
#include "../board/domain/events/BoardEvent.hpp"
#include "MissionParser.hpp"
#include "MissionStatus.hpp"
#include "Orchestrator.hpp"
#include "RobotEvent.hpp"
#include "RobotStatus.hpp"
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <memory>
#include <mutex>
#include <optional>
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
                    auto robot_task = missionParser_->parser(e.mission,operationMode_);
                    robot::domain::entities::RobotStatusCode temp;
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        temp = robotStatus_;
                    }
                    if(robot_task && temp == robot::domain::entities::RobotStatusCode::Idle)
                    {
                        // std::cout << "Da nhan mission.\n";
                        orchestrator_->enqueueMission(std::move(robot_task.value()));
                        robotEventBus_->publish(robot::domain::events::MissionAcceptedEvent{.mission_raw = e.mission});
                    }
                    else {
                        robotEventBus_->publish(robot::domain::events::MissionRejectedEvent{.mission_raw = e.mission});
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalCancelEvent>)
                {   
                    if(orchestrator_->getStepIndex() != -1)
                    {
                        std::cout << "signal cancel.\n";
                        orchestrator_->cancel();
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalPauseEvent>)
                {
                    if(orchestrator_->getStepIndex() != -1)
                    {
                        std::cout << "signal pause.\n";
                        orchestrator_->pause();
                        {
                            std::lock_guard<std::mutex> lk(mutexState_);
                            systemError_.pause_by_manual = true;
                        }
                        updateRobotStatus();
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalResumeEvent>)
                {     
                    if(orchestrator_->getStepIndex() != -1)
                    {
                        std::cout << "signal resume.\n";
                        orchestrator_->resume();
                        {
                            std::lock_guard<std::mutex> lk(mutexState_);
                            systemError_.pause_by_manual = false;
                        }
                        updateRobotStatus();
                    }
                    
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalSwitchModeEvent>)
                {     
                    Json::CharReaderBuilder builder;
                    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                    Json::Value root;
                    std::string errors; 
                    
                    bool is_parsed_success = reader->parse(
                        e.mode.c_str(), 
                        e.mode.c_str() + e.mode.length(), 
                        &root, 
                        &errors
                    );
                    
                    if (!is_parsed_success) {
                        std::cerr << "Lỗi Parse JSON switch mode: " << errors << std::endl;
                    }
                    else {
                        if(root.isMember("operation_mode") && root["operation_mode"].isInt())
                        {
                            std::lock_guard<std::mutex> lk(mutexState_);
                            if(root["operation_mode"].asInt() == static_cast<int>(robot::domain::entities::RobotOperationMode::Manual))
                            {
                                operationMode_ = robot::domain::entities::RobotOperationMode::Manual;
                            }
                            else if(root["operation_mode"].asInt() == static_cast<int>(robot::domain::entities::RobotOperationMode::Auto))
                            {
                                operationMode_ = robot::domain::entities::RobotOperationMode::Auto;
                            }
                        }
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalClearErrorEvent>)
                {     
                    std::cout << "signal clear error.\n";
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.mission_error = false;
                        systemError_.navigator_error = false;
                        systemError_.navigator_failed = false;
                    }
                    updateRobotStatus();
                    robotEventBus_->publish(robot::domain::events::RobotClearErrorEvent{});
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalCollisionEvent>)
                {     
                    std::cout << "signal collision.\n";
                    if(operationMode_ == robot::domain::entities::RobotOperationMode::Auto)
                    {
                        
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SwitchMapEvent>)
                {     
                    robot::domain::entities::RobotStatusCode temp;
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        temp = robotStatus_;
                    }
                    if(temp == robot::domain::entities::RobotStatusCode::Idle)
                    {
                        navigatorController_->switchMap(e.map_name);
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SetShelfEvent>)
                {
                    robot::domain::entities::RobotStatusCode temp;
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        temp = robotStatus_;
                    }
                    if(temp == robot::domain::entities::RobotStatusCode::Idle)
                    {
                        navigatorController_->setShelf(e.shelf_name);
                    }
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::ClearShelfEvent>)
                {
                    robot::domain::entities::RobotStatusCode temp;
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        temp = robotStatus_;
                    }
                    if(temp == robot::domain::entities::RobotStatusCode::Idle)
                    {
                        navigatorController_->clearShelf();
                    }
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
                        // std::cout << "nav set fatal\n";
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskClearFailedEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_failed = false;
                        // std::cout << "nav clear fatal\n";
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetErrorEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_task_running = false;
                        systemError_.navigator_error = true;
                        // std::cout << "nav set error\n";
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearErrorEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.navigator_error = false;
                        // std::cout << "nav clear error\n";
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
                    {
                        navigatorController_->cancel();
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
                else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusSetEmergencyEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.lift_emergency = true;
                    }
                    updateRobotStatus();
                }
                else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusClearEmergencyEvent>)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        systemError_.lift_emergency = false;
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

    std::string RobotController::parseRobotState()
    {
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

        robot::domain::entities::RobotStatusCode temp1;
        robot::domain::entities::MissionStatusCode temp2;
        {
            std::lock_guard<std::mutex> lk(mutexState_);
            temp1 = robotStatus_;
            temp2 = missionStatus_;
        }

        root["robot_status"] = static_cast<int>(temp1);
        root["navigator_ip"] = navigatorState.ip_address;

        Json::Value mission_state;
        mission_state["mission_code"] = orchestrator_->getMissionId();
        mission_state["step"] = orchestrator_->getStepIndex();
        mission_state["mission_status"] = static_cast<int>(temp2);
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
                pushMissionStatusResponse(mission_id, robot::domain::entities::MissionStatusCode::Cargo);
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
                pushMissionStatusResponse(mission_id, robot::domain::entities::MissionStatusCode::Completed);
                systemError_.pause_by_manual = false;
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
                pushMissionStatusResponse(mission_id, robot::domain::entities::MissionStatusCode::Cancel);
                systemError_.pause_by_manual = false;
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
                pushMissionStatusResponse(mission_id, robot::domain::entities::MissionStatusCode::Error);
                systemError_.pause_by_manual = false;
            }
            updateRobotStatus();
        });

        gatewayController_->setGetRobotStatusCallback([this](void){
            return parseRobotState();
        });
        orchestrator_->start();
        workerThread = std::thread(&RobotController::workerLoop,this);
        updateRobotStatus();
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
            auto response = popRobotResponse();
            if(response)
            {
                std::string payload = "";
                std::visit([&payload](const auto& res){
                    using T = std::decay_t<decltype(res)>;
                    if constexpr (std::is_same_v<T, MissionStatusResponse>)
                    {     
                        Json::Value root;
                        root["mission_status"] = static_cast<int>(res.mission_status);
                        root["mission_id"] = res.mission_id;

                        Json::StreamWriterBuilder builder;
                        builder["indentation"] = "";
                        payload = Json::writeString(builder, root);
                    }
                    else if constexpr (std::is_same_v<T, ErrorResponse>)
                    {     
                        Json::Value error;
                        error["error_code"] = res.error_code;
                        error["error_desc"] = res.error_desc;

                        Json::Value root;
                        root["robot_error"] = error;
                        Json::StreamWriterBuilder builder;
                        builder["indentation"] = "";
                        payload = Json::writeString(builder, root);
                    }
                }, *response);
                if(!payload.empty())
                {
#if POST_TO_FLEET == 1
                    while(1)
                    {
                        auto res = gatewayController_->sendResponse(payload);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        if(res.status == gateway::domain::entities::NetworkStatus::Success)
                            break;
                    }
#endif
                }
            }
#if POST_TO_FLEET == 1
            auto res = gatewayController_->sendStatus(parseRobotState());
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void RobotController::updateRobotStatus(void)
    {
        std::lock_guard<std::mutex> lk(mutexState_);
        if(systemError_.navigator_emergency || systemError_.navigator_error || systemError_.navigator_failed 
            || systemError_.navigator_fatal || systemError_.navigator_disconnected || systemError_.lift_error
            || systemError_.mission_error || systemError_.board_error || systemError_.lift_emergency)
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

    void RobotController::pushMissionStatusResponse(std::string mission_id,robot::domain::entities::MissionStatusCode mission_status)
    {
        std::lock_guard<std::mutex> lk(mutexResponseQueue_);
        robotResponseQueue_.emplace(MissionStatusResponse{.mission_id = mission_id,.mission_status = mission_status});
    }
    void RobotController::pushErrorResponse(int error_code, std::string error_desc)
    {
        std::lock_guard<std::mutex> lk(mutexResponseQueue_);
        robotResponseQueue_.emplace(ErrorResponse{error_code, std::move(error_desc)});
    }
    std::optional<RobotResponse> RobotController::popRobotResponse()
    {
        std::lock_guard<std::mutex> lk(mutexResponseQueue_);
        if(!robotResponseQueue_.empty())
        {
            auto response = robotResponseQueue_.front();
            robotResponseQueue_.pop();
            return response;
        }
        return std::nullopt;
    }
}