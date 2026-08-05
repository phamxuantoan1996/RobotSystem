#pragma once
#include <variant>
namespace common::ports {
    struct GotoStationStepResult {
        enum class Result {
            Success,
            Failed,
            Canceled
        };
        Result result;
    };
    struct UnknowStepResult {};
    using RobotStepResult = std::variant<
        UnknowStepResult,
        GotoStationStepResult
    >;
    class IRobotStep {
        public:
            virtual ~IRobotStep() = default;
            virtual RobotStepResult excute(RobotStepResult prevResult) = 0;
    };
}