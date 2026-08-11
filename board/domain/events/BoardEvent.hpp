#pragma once
#include <variant>

namespace board::domain::events {

    struct BoardDisconnectedEvent {};
    struct BoardReconnectedEvent {};

    using BoardEvent = std::variant<
        BoardDisconnectedEvent,
        BoardReconnectedEvent
    >;

}