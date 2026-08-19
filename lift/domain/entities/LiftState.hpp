#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
namespace lift::domain::entities {

    enum class LiftDeviceStatusCode {
        Unknown,
        Init,
        Idle,
        Busy,
        Error,
        Emergency
    };

    enum class LiftTaskStatusCode {
        Unknown,
        Running,
        Paused,
        Completed,
        Canceled
    };

    struct LiftState {
        LiftDeviceStatusCode device_status;
        LiftTaskStatusCode task_status;
        int16_t lift_position;
        std::vector<uint8_t> error_codes;
    };
}