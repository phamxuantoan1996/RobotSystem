#pragma once

#include <string>
#include <variant>
#include "../domain/entities/SignalType.hpp"
namespace gateway::domain::events {
    struct MissionDispatchEvent {
        std::string mission;
    };

    struct SignalCancelEvent {};
    struct SignalPauseEvent {};
    struct SignalResumeEvent {};
    struct SignalClearErrorEvent {};
    struct SignalSwitchModeEvent 
    {
        
    };
    struct SignalClearComodityEvent {};
    struct SignalCollisionEvent {
        gateway::domain::entities::CollisionSignalType collision_type;
    };


    struct ControlManualEvent {
        std::string control_manual;
    };

    struct SignalTransferEvent {
        gateway::domain::entities::TransferSignalType transfer_type;
    };

    using GatewayEvent = std::variant<
        MissionDispatchEvent,
        SignalCancelEvent,
        SignalPauseEvent,
        SignalResumeEvent,
        SignalClearErrorEvent,
        SignalSwitchModeEvent,
        SignalClearComodityEvent,
        SignalCollisionEvent,
        ControlManualEvent,
        SignalTransferEvent
    >;
}