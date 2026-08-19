#pragma once

#include <cstdint>
#include <string>
#include <variant>
namespace lift::domain::events {
    struct LiftStatusInitEvent {};
    struct LiftStatusIdleEvent {};
    struct LiftStatusBusyEvent {};
    struct LiftStatusSetErrorEvent {
        uint8_t error_code;
        std::string reason;
    };
    struct LiftStatusClearErrorEvent {
        uint8_t error_code;
    };

    struct LiftStatusSetEmergency {};
    struct LiftStatusClearEmergency {};

    struct LiftTaskRunningEvent {};
    struct LiftTaskCompleteEvent {};
    struct LiftTaskCancelEvent {};
    struct LiftTaskPausedEvent {
        uint8_t error_code;
        std::string reason;
    };

    using LiftEvent = std::variant<
        LiftStatusInitEvent,
        LiftStatusIdleEvent,
        LiftStatusBusyEvent,
        LiftStatusSetErrorEvent,
        LiftStatusClearErrorEvent,
        LiftStatusSetEmergency,
        LiftStatusClearEmergency,

        LiftTaskRunningEvent,
        LiftTaskCompleteEvent,
        LiftTaskCancelEvent,
        LiftTaskPausedEvent
    >;
}