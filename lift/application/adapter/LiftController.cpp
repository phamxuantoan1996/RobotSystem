#include "LiftController.hpp"
#include "../board/domain/entities/BoardCommand.hpp"
#include "../lift/domain/entities/LiftState.hpp"
#include "../lift/domain/events/LiftEvent.hpp"

#include <chrono>
#include <cstddef>
#include <future>
#include <iostream>
#include <jsoncpp/json/value.h>
#include <mutex>
#include <ostream>
#include <jsoncpp/json/json.h>

namespace lift::application::adapter {
    LiftController::LiftController(std::shared_ptr<board::domain::value_objects::BoardCommandQueue> board_command_queue)
    :boardCommandQueue_(std::move(board_command_queue))
    ,liftEventBus_(std::make_unique<common::application::EventBus<lift::domain::events::LiftEvent>>())
    {

    }

    std::error_code LiftController::liftMove(lift::domain::value_objects::LiftTarget target)
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>();

        boardCommandQueue_->enqueue(board::domain::entities::LiftCommand{
            .lift_command_type = board::domain::entities::LiftCommandType::LiftMove,
            .lift_target = target.getTarget(),
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

    std::error_code LiftController::init()
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>();

        boardCommandQueue_->enqueue(board::domain::entities::SystemCommand {
            .system_command_type = board::domain::entities::SystemCommandType::Init,
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

    std::error_code LiftController::pause()
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>();

        boardCommandQueue_->enqueue(board::domain::entities::SystemCommand {
            .system_command_type = board::domain::entities::SystemCommandType::Pause,
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

    std::error_code LiftController::resume()
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>();

        boardCommandQueue_->enqueue(board::domain::entities::SystemCommand {
            .system_command_type = board::domain::entities::SystemCommandType::Resume,
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

    std::error_code LiftController::cancel()
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto resolved = std::make_shared<std::atomic<bool>>();

        boardCommandQueue_->enqueue(board::domain::entities::SystemCommand {
            .system_command_type = board::domain::entities::SystemCommandType::Cancel,
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

    lift::domain::entities::LiftState LiftController::getState(void) const
    {
        std::lock_guard<std::mutex> lk(mutexState_);
        return cacheState_;
    }

    LiftController::HandlerId LiftController::subscribeEvents(LiftEventHandler handler)
    {
        return liftEventBus_->subscribe(std::move(handler));
    }

    void LiftController::unSubscribeEvents(LiftController::HandlerId id)
    {
        liftEventBus_->unsubscribe(id);
    }

    void LiftController::updateState(const std::string& raw_state)
    {
        // std::cout << "Update state : " << raw_state << std::endl;
        Json::Value root;
        Json::CharReaderBuilder builder;
        Json::CharReader* reader = builder.newCharReader();
        std::string errors;
        
        // Parse JSON
        bool success = reader->parse(
            raw_state.c_str(),
            raw_state.c_str() + raw_state.size(),
            &root,
            &errors
        );

        delete reader;

        if (!success) {
            std::cerr << "Failed to parse JSON: " << errors << std::endl;
        }
        else
        {
            
            lift::domain::entities::LiftState prev;
            lift::domain::entities::LiftState next;
            {
                std::lock_guard<std::mutex> lk(std::mutex);
                prev = prevSnapshot_;
                
                if (root.isMember("machine_state") && root["machine_state"].isInt()) {
                    int val = root["machine_state"].asInt();
                    cacheState_.device_status = static_cast<lift::domain::entities::LiftDeviceStatusCode>(val);
                } else {
                    std::cerr << "Missing or invalid field: system_state" << std::endl;
                }

                if (root.isMember("mission_state") && root["mission_state"].isInt()) {
                    int val = root["mission_state"].asInt();
                    cacheState_.task_status = static_cast<lift::domain::entities::LiftTaskStatusCode>(val);
                } else {
                    std::cerr << "Missing or invalid field: mission_state" << std::endl;
                }

                if (root.isMember("lift_position") && root["lift_position"].isInt()) {
                    cacheState_.lift_position = static_cast<int16_t>(root["lift_position"].asInt());
                } else {
                    std::cerr << "Missing or invalid field: lift_position" << std::endl;
                }

                if (root.isMember("error_code") && root["error_code"].isArray()) {
                    cacheState_.error_codes.clear();
                    for (const auto& item : root["error_code"]) 
                    {
                        if (item.isUInt()) {
                            cacheState_.error_codes.push_back(static_cast<uint8_t>(item.asUInt()));
                        }
                    }
                } 
                else 
                {
                    std::cerr << "Missing or invalid field: error_code" << std::endl;
                }
                next = cacheState_;
                prevSnapshot_ = cacheState_;
            }

            // detect event
            detectEvent(prev, next);
            // std::cout << raw_state << std::endl;
        }
    }

    void LiftController::detectEvent(const lift::domain::entities::LiftState& prev,const lift::domain::entities::LiftState& next)
    {
        // 1. Tìm các lỗi mới xuất hiện trong 'next' nhưng chưa có trong 'prev' để báo lỗi
        for (const auto& err : next.error_codes)
        {
            if (err != 0)
            {
                if (find(prev.error_codes.begin(), prev.error_codes.end(), err) == prev.error_codes.end()) 
                {
                    liftEventBus_->publish(lift::domain::events:: LiftStatusSetErrorEvent{.error_code = err});
                }
            }
        }

        // 2. Tìm các lỗi đã được xóa sạch (có trong 'prev' nhưng không còn trong 'next')
        for (const auto& err : prev.error_codes)
        {
            if (err != 0)
            {
                if (find(next.error_codes.begin(), next.error_codes.end(), err) == next.error_codes.end()) 
                {
                    liftEventBus_->publish(lift::domain::events::LiftStatusClearErrorEvent{.error_code = err});
                }
            }
        }

        if (prev.device_status != lift::domain::entities::LiftDeviceStatusCode::Emergency && next.device_status == lift::domain::entities::LiftDeviceStatusCode::Emergency) 
        {
            // liftEventBus_->publish(lift::domain::events::);
            std::cout << "lift set emergency\n";
        }
        else if(prev.device_status == lift::domain::entities::LiftDeviceStatusCode::Emergency && next.device_status != lift::domain::entities::LiftDeviceStatusCode::Emergency)
        {
            std::cout << "lift clear emergency\n";
        }

        
        if (prev.device_status != lift::domain::entities::LiftDeviceStatusCode::Init && next.device_status == lift::domain::entities::LiftDeviceStatusCode::Init) 
        {
            liftEventBus_->publish(lift::domain::events::LiftStatusInitEvent{});
        }
        else if (prev.device_status != lift::domain::entities::LiftDeviceStatusCode::Busy && next.device_status == lift::domain::entities::LiftDeviceStatusCode::Busy) 
        {
            liftEventBus_->publish(lift::domain::events::LiftStatusBusyEvent{});
        }
        else if (prev.device_status != lift::domain::entities::LiftDeviceStatusCode::Idle && next.device_status == lift::domain::entities::LiftDeviceStatusCode::Idle) 
        {
            liftEventBus_->publish(lift::domain::events::LiftStatusIdleEvent{});
        }
        


        // conveyor mission event
        if (prev.task_status != lift::domain::entities::LiftTaskStatusCode::Running && next.task_status == lift::domain::entities::LiftTaskStatusCode::Running) 
        {
            std::cout << "lift task running\n";
            liftEventBus_->publish(lift::domain::events::LiftTaskRunningEvent{});
        }
        else if (prev.task_status != lift::domain::entities::LiftTaskStatusCode::Completed && next.task_status == lift::domain::entities::LiftTaskStatusCode::Completed) 
        {
            std::cout << "list task completed\n";
            liftEventBus_->publish(lift::domain::events::LiftTaskCompleteEvent{});
        }
        else if (prev.task_status != lift::domain::entities::LiftTaskStatusCode::Canceled && next.task_status == lift::domain::entities::LiftTaskStatusCode::Canceled) 
        {
            std::cout << "list task canceled\n";
            liftEventBus_->publish(lift::domain::events::LiftTaskCancelEvent{});
        }
        else if (prev.task_status != lift::domain::entities::LiftTaskStatusCode::Paused && next.task_status == lift::domain::entities::LiftTaskStatusCode::Paused) 
        {
            std::cout << "list task paused\n";
            liftEventBus_->publish(lift::domain::events::LiftTaskPausedEvent{});
        }
        // error event
    }
}