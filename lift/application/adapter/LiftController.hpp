#pragma once

#include "../board/domain/value_objects/BoardCommandQueue.hpp"
#include "../lift/domain/events/LiftEvent.hpp"
#include "../lift/domain/entities/LiftState.hpp"
#include "../lift/domain/value_objects/LiftTarget.hpp"
#include "../common/ports/IEventBus.hpp"
#include "../common/application/EventBus.hpp"
#include <memory>
#include <mutex>
#include <system_error>

namespace lift::application::adapter {
    class LiftController {
        public:
            LiftController(std::shared_ptr<board::domain::value_objects::BoardCommandQueue> board_command_queue);
            LiftController(const LiftController& other) = delete;
            LiftController& operator=(const LiftController& other) = delete;

            std::error_code liftMove(lift::domain::value_objects::LiftTarget target);

            std::error_code init();
            std::error_code pause();
            std::error_code resume();
            std::error_code cancel();

            lift::domain::entities::LiftState getState(void) const;

            // callback duoc goi tu board controller de update state
            void updateState(const std::string& raw_state);

            // handle event
            using LiftEventHandler = std::function<void(const lift::domain::events::LiftEvent&)>;
            using HandlerId = typename common::ports::IEventBus<lift::domain::events::LiftEvent>::HandlerID;
            HandlerId subscribeEvents(LiftEventHandler handler);
            void unSubscribeEvents(HandlerId id);

        private:
            void detectEvent(const lift::domain::entities::LiftState& prev,const lift::domain::entities::LiftState& next);

            std::shared_ptr<board::domain::value_objects::BoardCommandQueue> boardCommandQueue_;
            std::unique_ptr<common::application::EventBus<lift::domain::events::LiftEvent>> liftEventBus_;

            lift::domain::entities::LiftState cacheState_;
            lift::domain::entities::LiftState prevSnapshot_;

            mutable std::mutex mutexState_;
    };
}