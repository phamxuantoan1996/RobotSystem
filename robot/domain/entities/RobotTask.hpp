#pragma once
#include "ports/IRobotStep.hpp"
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
        std::vector<std::unique_ptr<common::ports::IRobotStep>> steps;
    };
}