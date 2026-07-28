#pragma once
#include <string>
#include <variant>
namespace navigator::domain::events {
    // Connection events
    struct NavigatorDisconnectEvent {};
    struct NavigatorReconnectEvent {};

    // Block events
    struct NavigatorSetBlockEvent {};
    struct NavigatorClearBlockEvent {};

    // Emergency events
    struct NavigatorSetEmergencyEvent {};
    struct NavigatorClearEmergencyEvent {};

    // Arrived event
    struct NavigatorArrivedEvent {};

    // Exception events
    struct NavigatorSetErrorEvent {
        std::string code;
        std::string desc;
    };
    struct NavigatorClearErrorEvent {
        std::string code;
    };
    struct NavigatorSetFatalEvent {
        std::string code;
        std::string desc;
    };
    struct NavigatorClearFatalEvent {
        std::string code;
    };

    // Control events
    struct NavigatorTaskCanceledEvent {};
    struct NavigatorTaskStartedEvent {};
    struct NavigatorTaskPausedEvent {};
    struct NavigatorTaskResumedEvent {};
    struct NavigatorTaskSetFailedEvent {};
    struct NavigatorTaskClearFailedEvent {};
    struct NavigatorRelocationConfirmEvent {};

    using NavigatorEvent = std::variant<
        NavigatorDisconnectEvent,
        NavigatorReconnectEvent,
        NavigatorSetBlockEvent,
        NavigatorClearBlockEvent,
        NavigatorSetEmergencyEvent,
        NavigatorClearEmergencyEvent,
        NavigatorArrivedEvent,
        NavigatorSetErrorEvent,
        NavigatorClearErrorEvent,
        NavigatorSetFatalEvent,
        NavigatorClearFatalEvent,
        NavigatorTaskCanceledEvent,
        NavigatorTaskPausedEvent,
        NavigatorTaskSetFailedEvent,
        NavigatorTaskClearFailedEvent,
        NavigatorTaskResumedEvent,
        NavigatorRelocationConfirmEvent,
        NavigatorTaskStartedEvent
    >;


}