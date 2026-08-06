#pragma once
#include "ports/IRobotStep.hpp"
#include <string>
#include <vector>
namespace robot::domain::entities {
    struct RobotTask {
        std::string mission_id;
        std::vector<common::ports::IRobotStep> steps;
    };
}