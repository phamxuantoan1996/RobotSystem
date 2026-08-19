#include "../indicator/application/adapter/IndicatorController.hpp"
#include <cstdint>
#include <future>
#include <system_error>

namespace indicator::application::adapter {
    IndicatorController::IndicatorController(std::shared_ptr<board::domain::value_objects::BoardCommandQueue> board_command_queue)
    :boardCommandQueue_(board_command_queue)
    {

    }
    std::error_code IndicatorController::setColor(indicator::domain::entities::ColorType color)
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>();

        boardCommandQueue_->enqueue(board::domain::entities::IndicatorCommand {
            .indicator_command_type = board::domain::entities::IndicatorCommandType::SetColor,
            .color = static_cast<uint8_t>(color),
            .callback = [promise,resolved](bool success){
                if(resolved->exchange(true))
                {
                    return;
                }
                promise->set_value(success);
            }
        });

        auto status = future.wait_for(std::chrono::seconds(5));
        if(status != std::future_status::ready)
        {
            return std::make_error_code(std::errc::timed_out);
        }
        else {
            if(!future.get())
                return std::make_error_code(std::errc::timed_out);
        }
        return {};
    }
}