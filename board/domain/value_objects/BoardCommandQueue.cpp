#include "BoardCommandQueue.hpp"
#include <iostream>

namespace board::domain::value_objects {
    void BoardCommandQueue::enqueue(const board::domain::entities::BoardCommand& command)
    {
        std::lock_guard<std::mutex> lk(mutexQueue_);
        queue_.push(command);
    }

    std::optional<board::domain::entities::BoardCommand> BoardCommandQueue::tryDequeue()
    {
        std::lock_guard<std::mutex> lk(mutexQueue_);
        if(queue_.empty())
        {
            return std::nullopt;
        }
        auto command = std::move(queue_.front());
        queue_.pop();
        return command;
    }

    bool BoardCommandQueue::isEmpty()
    {
        std::lock_guard<std::mutex> lk(mutexQueue_);
        return queue_.empty();
    }

}