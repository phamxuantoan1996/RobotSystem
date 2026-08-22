#pragma once

namespace robot::domain::entities {
    enum class RobotOperationMode {
        Unknown = -1,
        Manual,
        Auto
    };

    enum class RobotStatusCode {
        Exception = -1,
        None = 0,
        Idle = 1,
        Active = 2,
        Stop = 3,
        PauseManual = 4,
        PauseCollison = 6,
        Error = 19
    };
}

