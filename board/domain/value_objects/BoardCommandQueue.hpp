#pragma once
#include "../board/domain/entities/BoardCommand.hpp"
#include <optional>
#include <queue>
#include <mutex>

namespace board::domain::value_objects {
    class BoardCommandQueue {
        private:
            mutable std::queue<board::domain::entities::BoardCommand> queue_;
            mutable std::mutex mutexQueue_;
        
        public:
            void enqueue(const board::domain::entities::BoardCommand& command);

            std::optional<board::domain::entities::BoardCommand> tryDequeue();

            bool isEmpty();
    };
}