#pragma once

#include "../lift/application/adapter/LiftController.hpp"
#include "../indicator/application/adapter/IndicatorController.hpp"
#include "../navigator/application/adapter/NavigatorController.hpp"
#include "../board/application/adapter/BoardController.hpp"
#include "../robot/application/controller/RobotController.hpp"
#include "../indicator/domain/entities/ColorType.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace reactor {
    struct IndicatorReactorState
    {
        /* data */
        bool navigator_emergency = false;
        bool navigator_disconnected = false;
        bool navigator_blocked = false;
        bool navigator_failed = false;
        bool navigator_fatal = false;
        bool navigator_error = false;
        bool navigator_task_running = false;

        bool lift_error = false;
        bool lift_task_running = false; 

        bool board_error = false;

        bool mission_running = false;
        bool mission_error = false;
    };
    class IndicatorReactor {
        public:
            explicit IndicatorReactor(std::shared_ptr<robot::application::RobotController> robot_controller,
                std::shared_ptr<board::application::adapter::BoardController> board_controller,
                std::shared_ptr<navigator::application::adapter::NavigatorController> navigator_controller,
                std::unique_ptr<indicator::application::adapter::IndicatorController> indicator_controller,
                std::shared_ptr<lift::application::adapter::LiftController> lift_controller);

            ~IndicatorReactor();
            
            void start(void);

            

        private:
            void onNavigatorEvent(const navigator::domain::events::NavigatorEvent& e);
            void onRobotEvent(const robot::domain::events::RobotEvent& e);
            void onBoardEvent(const board::domain::events::BoardEvent& e);
            void onLiftEvent(const lift::domain::events::LiftEvent& e);
            void updateLight(void);

            std::unique_ptr<indicator::application::adapter::IndicatorController> indicatorController_;
            std::mutex mutexState_;
            IndicatorReactorState indicatorReactorState_;
            indicator::domain::entities::ColorType currentColor_ = indicator::domain::entities::ColorType::Off;

            std::mutex mutexQueue_;
            std::queue<indicator::domain::entities::ColorType> colorQueue_;
            std::condition_variable cvQueue_;
            
            std::atomic<bool> running_{false};
            std::thread workerThread_;
            void workerLoop();
    };
}