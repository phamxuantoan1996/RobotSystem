#pragma once

#include <variant>
namespace lift::domain::events {
    struct LiftStatusInitEvent {};
    struct LiftStatusIdleEvent {};
    struct LiftStatusBusyEvent {};
    struct LiftStarusErrorEvent {};

    struct LiftTaskRunningEvent {};
    struct LiftTaskCompleteEvent {};
    struct LiftTaskCancelEvent {};
    struct LiftTaskErrorEvent {};

    using LiftEvent = std::variant<
        LiftStatusInitEvent,
        LiftStatusIdleEvent,
        LiftStatusBusyEvent,
        LiftStarusErrorEvent,

        LiftTaskRunningEvent,
        LiftTaskCompleteEvent,
        LiftTaskCancelEvent,
        LiftTaskErrorEvent
    >;
}