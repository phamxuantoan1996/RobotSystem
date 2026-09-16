#pragma once
#include "../joystick/ports/IJoyStick.hpp"
#include "../joystick/domain/entities/JoyStickSignal.hpp"
#include <mutex>
#include <string>

namespace joystick::drivers::plc {
    class JoyStickPLC : public joystick::ports::IJoyStick {
        public:
            explicit JoyStickPLC();
            void setJoyEventCallback(JoyStickEventCallback cb) override;
            void updateState(const std::string& raw_state);


        private:
            void detectEvent(const joystick::domain::entities::JoyStickSignal& prev, const joystick::domain::entities::JoyStickSignal& next);
            JoyStickEventCallback eventCallback_;

            joystick::domain::entities::JoyStickSignal cacheSignal_;
            joystick::domain::entities::JoyStickSignal prevSnapshot_;
            std::mutex mutexSignal_;
    };
}