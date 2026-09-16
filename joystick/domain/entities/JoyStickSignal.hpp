#pragma once

namespace joystick::domain::entities {
    struct JoyStickSignal {
        bool turn_left = false;
        bool turn_right = false;
        bool forward = false;
        bool backward = false;
        bool enable = false;
    };
}