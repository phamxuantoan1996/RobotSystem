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
        int code;
        std::string desc;
    };
    struct NavigatorClearErrorEvent {
        int code;
    };
    struct NavigatorSetFatalEvent {
        int code;
        std::string desc;
    };
    struct NavigatorClearFatalEvent {
        int code;
    };

    // Control events
    struct NavigatorCanceledEvent {};
    struct NavigatorPausedEvent {};
    struct NavigatorResumedEvent {};
    struct NavigatorRelocationEvent {};

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
        NavigatorCanceledEvent,
        NavigatorPausedEvent,
        NavigatorResumedEvent
    >;


}