#pragma once

#include <string>
#include <unordered_map>
namespace lift::domain::entities {
    enum class LiftErrorCode {
        ErrorNone,
        ErrorEmergency,
        ErrorTimeout
    };

    enum class LiftDeviceStatusCode {
        Unknown,
        Init,
        Idle,
        Busy,
        Error
    };

    enum class LiftTaskStatusCode {
        Unknow,
        Running,
        Completed,
        Canceled,
        Error
    };

    struct LiftState {
        LiftDeviceStatusCode device_status;
        LiftTaskStatusCode task_status;
        std::unordered_map<LiftErrorCode, std::string> errors;
    };
}