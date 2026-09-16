#pragma once
#include "../joystick/domain/events/JoyStickEvent.hpp"

#include <functional>
namespace joystick::ports {

    class IJoyStick {
        public:
            virtual ~IJoyStick() = default;

            using JoyStickEventCallback = std::function<void(const joystick::domain::events::JoyStickEvent & event)>;
            virtual void setJoyEventCallback(JoyStickEventCallback cb) = 0;
    };
}