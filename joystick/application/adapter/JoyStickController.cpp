#include "../joystick/application/adapter/JoyStickController.hpp"

namespace joystick::application::adapters {
    JoyStickController::JoyStickController(std::shared_ptr<joystick::ports::IJoyStick> driver) 
    : joystickEventBus_(std::make_unique<common::application::EventBus<joystick::domain::events::JoyStickEvent>>())
    , driver_(std::move(driver))
    {
        driver_->setJoyEventCallback([this](const joystick::domain::events::JoyStickEvent& event){
            handleEvent(event);
        });
    }
    JoyStickController::HandlerId JoyStickController::subscribeEvents(JoyStickEventHandler handler)
    {
        return joystickEventBus_->subscribe(handler);
    }
    void JoyStickController::unSubscribeEvents(JoyStickController::HandlerId id)
    {
        joystickEventBus_->unsubscribe(id);
    }
    void JoyStickController::handleEvent(const joystick::domain::events::JoyStickEvent& event)
    {
        joystickEventBus_->publish(event);
    }
}