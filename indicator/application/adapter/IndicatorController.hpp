#pragma once
#include "../board/domain/value_objects/BoardCommandQueue.hpp"
#include "../indicator/domain/entities/ColorType.hpp"
#include <memory>
#include <system_error>

namespace indicator::application::adapter {
    class IndicatorController {
        public:
            explicit IndicatorController(std::shared_ptr<board::domain::value_objects::BoardCommandQueue> board_command_queue);
            std::error_code setColor(indicator::domain::entities::ColorType color);
        private:
            std::shared_ptr<board::domain::value_objects::BoardCommandQueue> boardCommandQueue_;
    };
}