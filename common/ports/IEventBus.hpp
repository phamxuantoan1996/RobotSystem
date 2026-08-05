#pragma once
#include <cstdint>
#include <functional>

namespace common::ports {
    template <typename EventT>
    class IEventBus {
        public:
            using HandlerID = uint64_t;
            using Handler = std::function<void(const EventT&)>;

            virtual ~IEventBus() = default;
            virtual HandlerID subscribe(Handler handler) = 0;
            virtual void unsubscribe(HandlerID id) = 0;
            virtual void publish(const EventT& event) = 0;
    };
}