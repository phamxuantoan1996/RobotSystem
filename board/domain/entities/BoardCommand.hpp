#pragma once
#include <cstdint>
#include <functional>
#include <variant>

namespace board::domain::entities {
    enum class CommandType {
        Unknown = -1,
        System,
        Lift,

        Indicator = 10
    };

    enum class SystemCommandType {
        Poll,
        Pause,
        Resume,
        Cancel,
        Init
    };

    enum class LiftCommandType {
        Unknown = -1,
        LiftMove
    };

    enum class IndicatorCommandType {
        Unknown,
        SetColor
    };

    struct SystemCommand {
        SystemCommandType system_command_type;
        std::function<void(bool)> callback;
    };

    struct LiftCommand {
        LiftCommandType lift_command_type;
        uint16_t lift_target;
        std::function<void(bool)> callback;
    };

    struct IndicatorCommand {
        IndicatorCommandType indicator_command_type;
        uint8_t color;
        std::function<void(bool)> callback;
    };



    using BoardCommand = std::variant<
        SystemCommand,
        LiftCommand,
        IndicatorCommand
    >;
}