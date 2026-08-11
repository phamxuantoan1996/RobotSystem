#include "BoardCommandBuilder.hpp"
#include "../board/domain/entities/BoardCommand.hpp"
#include <cstdint>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <optional>
#include <string>


namespace board::domain::value_objects {

    std::optional<std::string> BoardCommandBuilder::systemBuildCommand(board::domain::entities::SystemCommand command)
    {
        std::string command_str;
        Json::Value root;

        root["type_id"] = 0;
        root["command_id"] = static_cast<uint8_t>(command.system_command_type);

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";  // Nếu muốn JSON không xuống dòng
        command_str = Json::writeString(builder, root);

        return command_str;
    }

    std::optional<std::string> BoardCommandBuilder::liftBuildCommand(board::domain::entities::LiftCommand command)
    {
        std::string command_str;
        Json::Value root;
        root["type_id"] = static_cast<uint8_t>(board::domain::entities::CommandType::Lift);
        root["command_id"] = static_cast<uint8_t>(command.lift_command_type);
        
        Json::Value params;
        params["lift_target"] = static_cast<uint16_t>(command.lift_target);
        root["params"] = params;

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";  // Nếu muốn JSON không xuống dòng
        command_str = Json::writeString(builder, root);
        return command_str;
    }

    std::optional<std::string> BoardCommandBuilder::indicatorBuildCommand(board::domain::entities::IndicatorCommand command)
    {
        std::string command_str;
        Json::Value root;
        root["type_id"] = static_cast<uint8_t>(board::domain::entities::CommandType::Indicator);
        root["command_id"] = static_cast<uint8_t>(command.indicator_command_type);
        
        Json::Value params;
        params["color"] = static_cast<uint8_t>(command.color);
        root["params"] = params;

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";  // Nếu muốn JSON không xuống dòng
        command_str = Json::writeString(builder, root);
        return command_str;
    }

}