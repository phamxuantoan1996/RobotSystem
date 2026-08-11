#include "../robot/application/controller/RobotController.hpp"
#include "../common/application/EventBus.hpp"
#include "MissionParser.hpp"
#include "Orchestrator.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace robot::application {
    RobotController::RobotController(std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController,
        std::unique_ptr<gateway::application::adapter::GatewayController> gatewayController)
    :navigatorController_(navigatorController),
    gatewayController_(std::move(gatewayController)),
    robotEventBus_(std::make_unique<common::application::EventBus<robot::domain::events::RobotEvent>>()),
    missionParser_(std::make_unique<robot::domain::value_objects::MissionParser>(navigatorController_)),
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
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalPauseEvent>)
                {     
                    std::cout << "signal pause.\n";
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalResumeEvent>)
                {     
                    std::cout << "signal resume.\n";
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalSwitchModeEvent>)
                {     
                    std::cout << "signal switch mode.\n";
                }
                else if constexpr (std::is_same_v<T, gateway::domain::events::SignalClearErrorEvent>)
                {     
                    std::cout << "signal clear error.\n";
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

    }
    RobotController::~RobotController()
    {
        stop();
    }

    RobotController::HandlerId RobotController::subcribeEvents(RobotController::RobotEventHandler handler)
    {
        return robotEventBus_->subscribe(std::move(handler));
    }
    void RobotController::unSubcribeEvents(RobotController::HandlerId id)
    {
        robotEventBus_->unsubscribe(id);
    }

    void RobotController::start()
    {
        orchestrator_->setMissionRunningCallback([](const std::string& mission_id){
            std::cout << "Mission running : " << mission_id << std::endl;
        });
        orchestrator_->setMissionCompletedCallback([](const std::string& mission_id){
            std::cout << "Mission completed : " << mission_id << std::endl;
        });
        orchestrator_->setMissionCanceledCallback([](const std::string& mission_id){
            std::cout << "Mission canceled : " << mission_id << std::endl;
        });
        orchestrator_->setMissionErrorCallback([](const std::string& mission_id){
            std::cout << "Mission error : " << mission_id << std::endl;
        });

        orchestrator_->start();
        running_ = true;
        while (running_) {
            std::cout << "step index : " << orchestrator_->getStepIndex() << std::endl;
            std::cout << "task status : " << static_cast<int>(orchestrator_->getTaskStatus()) << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    void RobotController::stop()
    {
        running_ = false;
    }
}