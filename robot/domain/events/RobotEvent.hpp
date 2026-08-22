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

    struct MissionRunningEvent {
        std::string mission_id;
    };

    struct MissionErrorEvent {
        std::string mission_id;
    };

    struct MissionCanceledEvent {
        std::string mission_id;
    };
    struct MissionCompletedEvent {
        std::string mission_id;
    };

    

    using RobotEvent = std::variant<
        MissionAcceptedEvent,
        MissionRejectedEvent,
        MissionRunningEvent,
        MissionErrorEvent,
        MissionCanceledEvent,
        MissionCompletedEvent
    >;
}