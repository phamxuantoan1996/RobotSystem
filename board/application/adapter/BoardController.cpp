#include "BoardController.hpp"
#include "../board/domain/value_objects/BoardCommandBuilder.hpp"
#include "../board/domain/entities/BoardCommand.hpp"
#include "../board/domain/value_objects/BoardCommandQueue.hpp"
#include "../board/domain/events/BoardEvent.hpp"
#include <variant>
#include <cstdint>
#include <iostream>
#include <system_error>
#include <thread>

namespace board::application::adapter {
    BoardController::BoardController(std::unique_ptr<board::ports::IBoardTransport> driver,std::shared_ptr<board::domain::value_objects::BoardCommandQueue> command_queue,uint32_t poll_inter_val_ms)
    : driver_(std::move(driver)),
    commandQueue_(std::move(command_queue)),
    boardEventBus_(std::make_unique<common::application::EventBus<board::domain::events::BoardEvent>>()),
    pollIntervalMs(poll_inter_val_ms)
    {

    }

    BoardController::~BoardController() {
        disconnect();
    }

    std::error_code BoardController::connect()
    {
        bool expected = false;
        if(!connected_.compare_exchange_strong(expected,true))
        {
            running_ = false;
            driver_->disconnect();
            if (pollThread.joinable()) {
                pollThread.join();
            }
        }
        if(auto ec = driver_->connect())
        {
            return ec;
        }


        connected_ = true;
        running_ = true;
        pollThread = std::thread(&BoardController::pollLoop,this);
        return {};
    }

    void BoardController::disconnect()
    {
        running_ = false;
        if(pollThread.joinable())
        {
            pollThread.join();
        }
        driver_->disconnect();
    }

    bool BoardController::isConnected() const {
        return connected_;
    }

    void BoardController::setCallbackUpdateState(CallbackUpdateState cb)
    {
        callbackUpdateState_.push_back(std::move(cb));
    }

    void BoardController::pollLoop(void)
    {
        uint8_t timeout_count = 0;
        while (running_) {
            auto command = commandQueue_->tryDequeue();
            if(command)
            {
                std::visit([this](auto&& cmd) {
                    using T = std::decay_t<decltype(cmd)>;
                    if constexpr (std::is_same_v<T, board::domain::entities::LiftCommand>) {
                        bool success = false;
                        auto conveyor_command = board::domain::value_objects::BoardCommandBuilder::liftBuildCommand(cmd);
                        if(conveyor_command)
                        {
                            auto ec = driver_->write(*conveyor_command, BOARD_POLL_STATE_TIMEOUT);
                            if(!ec)
                            {
                                std::string res = "";
                                ec = driver_->readUntil(res,'/',BOARD_POLL_STATE_TIMEOUT);
                                if(!ec)
                                {
                                    success = true;
                                }
                            }
                        }
                        if(cmd.callback)
                        {
                            cmd.callback(success);
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
                    }
                    else if constexpr (std::is_same_v<T, board::domain::entities::SystemCommand>) {
                        bool success = false;
                        auto system_command = board::domain::value_objects::BoardCommandBuilder::systemBuildCommand(cmd);
                        std::cout << system_command.value() << std::endl;
                        if(system_command)
                        {
                            auto ec = driver_->write(*system_command, BOARD_POLL_STATE_TIMEOUT);
                            if(!ec)
                            {
                                std::string res = "";
                                ec = driver_->readUntil(res,'/',BOARD_POLL_STATE_TIMEOUT);
                                if(!ec)
                                {
                                    success = true;
                                }
                            }
                        }
                        if(cmd.callback)
                        {
                            cmd.callback(success);
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
                    }
                    else if constexpr (std::is_same_v<T, board::domain::entities::IndicatorCommand>) {
                        bool success = false;
                        auto indicator_command = board::domain::value_objects::BoardCommandBuilder::indicatorBuildCommand(cmd);
                        // std::cout << indicator_command.value() << std::endl;
                        if(indicator_command)
                        {
                            auto ec = driver_->write(*indicator_command, BOARD_POLL_STATE_TIMEOUT);
                            if(!ec)
                            {
                                std::string res = "";
                                ec = driver_->readUntil(res,'/',BOARD_POLL_STATE_TIMEOUT);
                                if(!ec)
                                {
                                    success = true;
                                }
                            }
                        }
                        if(cmd.callback)
                        {
                            cmd.callback(success);
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
                    }
                }, *command);
                continue;   
            }
            
            auto poll_command = board::domain::value_objects::BoardCommandBuilder::systemBuildCommand(board::domain::entities::SystemCommand{.system_command_type = board::domain::entities::SystemCommandType::Poll});
            if(poll_command)
            {
                auto ec = driver_->write(*poll_command,BOARD_POLL_STATE_TIMEOUT);
                if (ec) {
                    timeout_count++;
                    if(timeout_count == 5)
                    {
                        // pushlish disconnected event
                        timeout_count = 6;
                    }
                    continue;
                }
                std::string res = "";
                ec = driver_->readUntil(res,'/',BOARD_POLL_STATE_TIMEOUT);
                // std::cout << "res : " << res << std::endl;
                if(ec)
                {
                    if(timeout_count == 5)
                    {
                        // publish disconnected event
                        boardEventBus_->publish(board::domain::events::BoardDisconnectedEvent{});
                        timeout_count = 6;
                    }
                    else {
                        timeout_count++;
                    }
                    continue;
                }
                else {
                    if(timeout_count == 6)
                    {
                        boardEventBus_->publish(board::domain::events::BoardReconnectedEvent{});
                    }
                    timeout_count = 0;
                    for(auto cb : callbackUpdateState_)
                    {
                        cb(res);
                    }
                    // publish reconnected event
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
        }
        
    }

    BoardController::HandlerId BoardController::subscribeEvents(BoardEventHandler handler)
    {
        return boardEventBus_->subscribe(std::move(handler));
    }

    void BoardController::unSubscribeEvents(BoardController::HandlerId id)
    {
        boardEventBus_->unsubscribe(id);
    }
}