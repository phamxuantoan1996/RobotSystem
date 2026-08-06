#pragma once
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include "../robot/domain/entities/RobotTask.hpp"
namespace robot::domain::value_objects {
    class RobotTaskQueue {
        public:
            void enqueue(robot::domain::entities::RobotTask task);
            std::optional<robot::domain::entities::RobotTask> dequeue();

            void clear();
            bool isEmpty();
            std::size_t size();
        private:
            std::queue<robot::domain::entities::RobotTask> queue_;
            std::mutex mutexQueue_;
    };
}