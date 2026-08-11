#pragma once

#include "../board/domain/entities/BoardCommand.hpp"
#include <optional>
#include <string>
namespace board::domain::value_objects {
    class BoardCommandBuilder {
        public:
            static std::optional<std::string> systemBuildCommand(board::domain::entities::SystemCommand command);
            static std::optional<std::string> liftBuildCommand(board::domain::entities::LiftCommand command);
            static std::optional<std::string> indicatorBuildCommand(board::domain::entities::IndicatorCommand command);
    };
}