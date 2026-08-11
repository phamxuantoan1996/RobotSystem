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
            explicit IRobotStep(int action_index) : actionIndex_(action_index) {}
            virtual ~IRobotStep() = default;
            virtual RobotStepResult excute(RobotStepResult prevResult) = 0;
            virtual int getActionIndex() = 0;
        protected:
            int actionIndex_ = -1;
    };
}