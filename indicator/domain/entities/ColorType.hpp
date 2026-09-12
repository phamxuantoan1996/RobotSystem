#pragma once

namespace indicator::domain::entities {
    enum class ColorType {
        Off = 0,
        Green = 2,
        GreenBlink = 13,
        Yellow = 10,
        YellowBlink = 11,
        Red = 8,
        RedBlink = 9,
        Purple = 15
    };
}