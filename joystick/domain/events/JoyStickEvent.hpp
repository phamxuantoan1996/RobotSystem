#pragma once

#include <variant>
namespace joystick::domain::events {
    struct JoyStickSetTurnLeftEvent {};
    struct JoyStickClearTurnLeftEvent {};
    struct JoyStickSetTurnRightEvent {};
    struct JoyStickClearTurnRightEvent {};
    struct JoyStickSetForwardEvent {};
    struct JoyStickClearForwardEvent {};
    struct JoyStickSetBackwardEvent {};
    struct JoyStickClearBackwardEvent {};
    struct JoyStickEnableEvent {};
    struct JoyStickDisableEvent {};

    using JoyStickEvent = std::variant<
        JoyStickSetTurnLeftEvent,
        JoyStickClearTurnLeftEvent,
        JoyStickSetTurnRightEvent,
        JoyStickClearTurnRightEvent,
        JoyStickSetBackwardEvent,
        JoyStickClearBackwardEvent,
        JoyStickSetForwardEvent,
        JoyStickClearForwardEvent,
        JoyStickEnableEvent,
        JoyStickDisableEvent>;
}