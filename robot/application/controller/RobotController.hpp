#pragma once
#include "../navigator/application/adapter/NavigatorController.hpp"
#include "../gateway/application/adapter/GatewayController.hpp"
#include "../common/ports/IEventBus.hpp"
#include "../robot/domain/events/RobotEvent.hpp"
#include "MissionParser.hpp"
#include "Orchestrator.hpp"
#include "RobotStatus.hpp"
#include "RobotTask.hpp"
#include <atomic>
#include <memory>
#include <mutex>
namespace robot::application {
    class RobotController {
        public:
            RobotController(std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController,
                std::unique_ptr<gateway::application::adapter::GatewayController> gatewayController);
            ~RobotController();

            using RobotEventHandler = std::function<void(const robot::domain::events::RobotEvent&)>;
            using HandlerId = typename common::ports::IEventBus<robot::domain::events::RobotEvent>::HandlerID;
            HandlerId subcribeEvents(RobotEventHandler handler);
            void unSubcribeEvents(HandlerId id);

            void start();
            void stop();
        private:
            std::atomic<bool> running_;

            std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController_;
            std::unique_ptr<gateway::application::adapter::GatewayController> gatewayController_;
            std::unique_ptr<common::application::EventBus<robot::domain::events::RobotEvent>> robotEventBus_;

            std::unique_ptr<robot::domain::value_objects::MissionParser> missionParser_;
            std::unique_ptr<robot::application::Orchestrator> orchestrator_;

            std::mutex mutexState;
            robot::domain::entities::RobotStatusCode robotStatus_ = robot::domain::entities::RobotStatusCode::Exception;
            robot::domain::entities::RobotOperationMode operationMode_ = robot::domain::entities::RobotOperationMode::Manual;
    };
}