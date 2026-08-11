#include "MissionParser.hpp"
#include "RobotStatus.hpp"
#include "../navigator/application/use_cases/GoToStationStep.hpp"
#include "../navigator/domain/value_objects/station.hpp"
#include "RobotTask.hpp"
#include <iostream>
#include <json/value.h>
#include <memory>
#include <optional>
#include <jsoncpp/json/json.h>
#include <string>

namespace robot::domain::value_objects {
    MissionParser::MissionParser(std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController)
    : navigatorController_(navigatorController)
    {

    }

    std::optional<robot::domain::entities::RobotTask> MissionParser::parser(std::string mission_raw)
    {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errors;

        // 3. Parse the string
        bool isParsed = reader->parse(
            mission_raw.c_str(), 
            mission_raw.c_str() + mission_raw.size(), 
            &root, 
            &errors
        );
        std::string mission_code = "";
        domain::entities::RobotOperationMode activity_type;
        std::vector<std::unique_ptr<common::ports::IRobotStep>> steps;

        // 4. Extract data if parsing succeeded
        if (isParsed)
        {
            if(root["mission_id"].isString() && root["activity_type"].isInt() && root["action_list"].isArray())
            {
                mission_code = root["mission_id"].asString();
                activity_type = static_cast<domain::entities::RobotOperationMode>(root["activity_type"].asInt());
                const Json::Value action_list = root["action_list"];
                int action_index = 0;
                for (const auto& action : action_list) {
                    if(!action["name"].isString())
                    {
                        mission_code = "";
                        break;
                    }
                    std::string action_name = action["name"].asString();
                    if(action_name == "action_navigation")
                    {
                        if(!action["params"].isObject())
                        {
                            mission_code = "";
                            break;
                        }
                        const Json::Value&  params = action["params"];
                        if(!params["navigation_point"].isString())
                        {
                            mission_code = "";
                            break;
                        }
                        
                        try {
                            navigator::domain::value_objects::Station station(params["navigation_point"].asString());
                            auto step = std::make_unique<navigator::application::use_cases::GoToStationStep>(navigatorController_,station,action_index);
                            steps.push_back(std::move(step));
                        } 
                        catch (const std::string& error_msg) {
                            std::cerr << "Error: " << error_msg << "\n";
                            mission_code = "";
                            break;
                        }
                    }
                    else if (action_name == "action_lift")
                    {
                        /*
                        {
                            "name":"action_lift",
                            "params": {
                                "lift_point": "LM123",
                                "lift_target": 0
                            }
                        }
                        */

                    }
                    else {
                        mission_code = "";
                        break;
                    } 
                    action_index++;
                }
                if(!mission_code.empty())
                {
                    return domain::entities::RobotTask{
                        .mission_id = mission_code,
                        .activity_type = activity_type,
                        .steps = std::move(steps)
                    };
                }
            }
        }
        return std::nullopt;
    }
}