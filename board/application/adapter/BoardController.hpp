#pragma once
#include "../board/domain/value_objects/BoardCommandQueue.hpp"
#include "../board/domain/events/BoardEvent.hpp"
#include "../board/ports/IBoardTransport.hpp"
#include "../common/ports/IEventBus.hpp"
#include "../common/application/EventBus.hpp"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <system_error>
#include <thread>
#include <vector>

#define BOARD_POLL_STATE_TIMEOUT 3000 // ms

namespace board::application::adapter {
    class BoardController {
        public:
            explicit BoardController(std::unique_ptr<board::ports::IBoardTransport> driver,std::shared_ptr<board::domain::value_objects::BoardCommandQueue> command_queue,uint32_t poll_interval_ms);
            ~BoardController();

            // disable copy struct
            BoardController(const BoardController& other) = delete;
            BoardController& operator=(const BoardController& other) = delete;

            std::error_code connect();
            void disconnect(void);
            bool isConnected(void) const;

            using CallbackUpdateState = std::function<void(const std::string&)>;
            void setCallbackUpdateState(CallbackUpdateState cb);

            using BoardEventHandler = std::function<void(const board::domain::events::BoardEvent&)>;
            using HandlerId = typename common::ports::IEventBus<board::domain::events::BoardEvent>::HandlerID;
            HandlerId subscribeEvents(BoardEventHandler handler);
            void unSubscribeEvents(HandlerId id);

        private:
            std::thread pollThread;
            void pollLoop(void);

            std::atomic<bool> connected_{false};
            std::atomic<bool> running_{false};

            std::unique_ptr<board::ports::IBoardTransport> driver_;
            std::shared_ptr<board::domain::value_objects::BoardCommandQueue> commandQueue_;

            std::vector<CallbackUpdateState> callbackUpdateState_;

            std::unique_ptr<common::application::EventBus<board::domain::events::BoardEvent>> boardEventBus_;

            uint32_t pollIntervalMs = 100;
    };
}