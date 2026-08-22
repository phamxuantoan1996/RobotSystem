#pragma once
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <utility>
#include "../ports/IEventBus.hpp"

namespace common::application {
    template<typename EventT>
    class EventBus : public common::ports::IEventBus<EventT>
    {
        public:
            using HandlerID = typename common::ports::IEventBus<EventT>::HandlerID;
            using Handler = typename common::ports::IEventBus<EventT>::Handler;
            HandlerID subscribe(Handler handler) override
            {
                std::lock_guard<std::mutex> lk(mutex_); // lock to add handler into handler list
                auto id = next_id_++;
                handlers_[id] = std::move(handler);
                return id;
            }

            void unsubscribe(HandlerID id) override
            {
                std::lock_guard<std::mutex> lk(mutex_); // lock to delete handler from handler list
                handlers_.erase(id);
            }

            void publish(const EventT& event) override
            {
                std::unordered_map<HandlerID, Handler> snapShot;
                {
                    std::lock_guard<std::mutex> lk(mutex_); // lock to copy handler list
                    snapShot = handlers_;
                }
                for(auto &[key,handler] : snapShot) // Call all handlers in the list.
                {
                    handler(event);
                }
            }
        private:
            std::unordered_map<HandlerID, Handler> handlers_; // handler list
            std::mutex mutex_;
            HandlerID next_id_ = 0;
    };
}