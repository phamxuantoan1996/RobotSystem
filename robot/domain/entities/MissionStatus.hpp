#pragma once

namespace robot::domain::entities {
    enum class MissionStatusCode {
        Error = -1,
        Unknown = 0,
        Cargo = 18,
        Cancel = 25,
        Completed = 29
    };
}