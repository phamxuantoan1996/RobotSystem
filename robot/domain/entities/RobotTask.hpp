#pragma once
#include "../robot/domain/entities/RobotStatus.hpp"
#include "../common/ports/IRobotStep.hpp"
#include <memory>
#include <string>
#include <vector>
namespace robot::domain::entities {
    enum class RobotTaskStatus {
        Unknown,
        Running,
        Completed,
        Canceled,
        Error
    };
    struct RobotTask {
        std::string mission_id;
        domain::entities::RobotOperationMode activity_type;
        std::vector<std::unique_ptr<common::ports::IRobotStep>> steps;
    };
}