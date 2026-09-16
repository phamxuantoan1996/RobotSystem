#pragma once
#include "../joystick/domain/events/JoyStickEvent.hpp"
#include "../joystick/ports/IJoyStick.hpp"
#include "../common/ports/IEventBus.hpp"
#include "../common/application/EventBus.hpp"

#include <functional>
#include <memory>
namespace joystick::application::adapters {
    class JoyStickController {
        public:
            explicit JoyStickController(std::shared_ptr<joystick::ports::IJoyStick> driver);
            using JoyStickEventHandler = std::function<void(const joystick::domain::events::JoyStickEvent)>;
            using HandlerId = typename common::ports::IEventBus<joystick::domain::events::JoyStickEvent>::HandlerID;
            HandlerId subscribeEvents(JoyStickEventHandler handler);
            void unSubscribeEvents(HandlerId id);
        private:
            void handleEvent(const joystick::domain::events::JoyStickEvent& event);
            std::unique_ptr<common::application::EventBus<joystick::domain::events::JoyStickEvent>> joystickEventBus_;
            std::shared_ptr<joystick::ports::IJoyStick> driver_;
    };
}