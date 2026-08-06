#include "RobotTaskQueue.hpp"
#include "../robot/domain/entities/RobotTask.hpp"

namespace robot::domain::value_objects {
    void RobotTaskQueue::enqueue(robot::domain::entities::RobotTask task)
    {
        // std::lock_guard<std::mutex> lk(mutexQueue_);
        queue_.push(std::move(task));
    }
    std::optional<robot::domain::entities::RobotTask> RobotTaskQueue::dequeue()
    {
        // std::lock_guard<std::mutex> lk(mutexQueue_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        auto task = std::move(queue_.front());
        queue_.pop();
        return task;
    }

    void RobotTaskQueue::clear()
    {
        // std::lock_guard<std::mutex> lk(mutexQueue_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }
    bool RobotTaskQueue::isEmpty()
    {
        // std::lock_guard<std::mutex> lk(mutexQueue_);
        return  queue_.empty();
    }
    std::size_t RobotTaskQueue::size()
    {
        // std::lock_guard<std::mutex> lk(mutexQueue_);
        return queue_.size();
    }
}