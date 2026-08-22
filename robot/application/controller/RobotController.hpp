#pragma once
#include "../navigator/application/adapter/NavigatorController.hpp"
#include "../gateway/application/adapter/GatewayController.hpp"
#include "../common/ports/IEventBus.hpp"
#include "../robot/domain/events/RobotEvent.hpp"
#include "../robot/domain/value_objects/MissionParser.hpp"
#include "../robot/application/orchestrator/Orchestrator.hpp"
#include "../robot/domain/entities/RobotStatus.hpp"
#include "../robot/domain/entities/RobotTask.hpp"
#include "../lift/application/adapter/LiftController.hpp"
#include "../board/application/adapter/BoardController.hpp"
#include "../robot/domain/entities/MissionStatus.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
namespace robot::application {
    
    struct  RobotSystemError {
        bool navigator_emergency;
        bool navigator_disconnected = false;
        bool navigator_blocked = false;
        bool navigator_failed = false;
        bool navigator_fatal = false;
        bool navigator_error = false;
        bool navigator_task_running = false;

        bool pause_by_manual = false;

        bool lift_error = false;
        bool lift_task_running = false;

        bool mission_error = false;
        bool mission_running = false;

        bool board_error = false;
    };
    class RobotController {
        public:
            RobotController(std::shared_ptr<board::application::adapter::BoardController> boardController,
                std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController,
                std::shared_ptr<gateway::application::adapter::GatewayController> gatewayController,
                std::shared_ptr<lift::application::adapter::LiftController> liftController);
            ~RobotController();

            using RobotEventHandler = std::function<void(const robot::domain::events::RobotEvent&)>;
            using HandlerId = typename common::ports::IEventBus<robot::domain::events::RobotEvent>::HandlerID;
            HandlerId subscribeEvents(RobotEventHandler handler);
            void unSubscribeEvents(HandlerId id);

            void start();
            void stop();
        private:
            

            std::shared_ptr<board::application::adapter::BoardController> boardController_;
            std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController_;
            std::shared_ptr<gateway::application::adapter::GatewayController> gatewayController_;
            std::shared_ptr<lift::application::adapter::LiftController> liftController_;
            std::unique_ptr<common::application::EventBus<robot::domain::events::RobotEvent>> robotEventBus_;

            std::unique_ptr<robot::domain::value_objects::MissionParser> missionParser_;
            std::unique_ptr<robot::application::Orchestrator> orchestrator_;

            std::mutex mutexState_;
            robot::domain::entities::RobotStatusCode robotStatus_ = robot::domain::entities::RobotStatusCode::Exception;
            robot::domain::entities::RobotOperationMode operationMode_ = robot::domain::entities::RobotOperationMode::Manual;
            robot::domain::entities::MissionStatusCode missionStatus_ = robot::domain::entities::MissionStatusCode::Unknown;

            RobotSystemError systemError_;    
            
            void updateRobotStatus(void);
            std::atomic<bool> running_;
            std::thread workerThread;
            void workerLoop();
    };
}