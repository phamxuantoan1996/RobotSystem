#pragma once
#include <string>
#include <variant>
namespace  robot::domain::events {
    struct MissionAcceptedEvent {
        std::string mission_raw;
    };

    struct MissionRejectedEvent {
        std::string mission_raw;
        std::string reason;
    };

    struct MissionRunningEvent {};

    struct MissionErrorEvent {
        std::string reason;
    };

    struct MissionCanceledEvent {};
    struct MissionCompletedEvent {};

    

    using RobotEvent = std::variant<
        MissionAcceptedEvent,
        MissionRejectedEvent,
        MissionRunningEvent,
        MissionErrorEvent,
        MissionCanceledEvent,
        MissionRunningEvent
    >;
}