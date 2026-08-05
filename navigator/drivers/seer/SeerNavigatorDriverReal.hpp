#pragma once
#include "INavigatorDriver.hpp"
#include "ISeerNavigatorConnection.hpp"
#include "NavigatorState.hpp"
#include "SeerNavigatorCommandBuilder.hpp"
#include "SeerNavigatorFrameCodec.hpp"
#include "SeerNavigatorStateMapper.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
namespace navigator::drivers::seer {

    struct SeerNavigatorDriverConfigParams {
        std::string host = "192.168.192.5";
        uint16_t statusPort = 19204;
        uint16_t navPort = 19206;
        uint16_t controlPort = 19205;
        uint16_t configPort = 19207;
        uint16_t otherPort = 19210;
        uint32_t timeout = 3000; // miliseconds
        uint32_t pollStatusIntervals = 100; // miliseconds
        std::string mode = "real"; // real, sim, test
    };

    class SeerNavigatorDriverReal : public ports::INavigatorDriver {
        public:
            explicit SeerNavigatorDriverReal(SeerNavigatorDriverConfigParams configParams);
            ~SeerNavigatorDriverReal() override;

            std::error_code connect() override;
            void disconnect() override;
            bool isConnected() const override;

            std::error_code goToStation(const navigator::domain::value_objects::Station& station) override;
            std::error_code goToPoint(const navigator::domain::value_objects::Location& location,domain::entities::NavigatorBackMode backMode, domain::entities::NavigatorCoordinate navigatorCoordinate) override;

            std::error_code cancelNavigation() override;
            std::error_code pauseNavigation() override;
            std::error_code resumeNavigation() override;

            std::error_code relocation(const navigator::domain::value_objects::Location& location) override;
            std::error_code confirmRelocation() override;

            navigator::domain::entities::NavigatorState getState() const override;
            void setNavigatorEventCallback(NavigatorEventCallback cb) override;
        private:
            // background poll loop
            void workerTask();

            // send Navigation Command + await ACK
            std::error_code sendSeerNavigationCommand(const navigator::drivers::seer::SeerNavigatorFrame & req, uint16_t expectedResType);

            // send Control Command + await ACK
            std::error_code sendControlCommand(const navigator::drivers::seer::SeerNavigatorFrame& req, uint16_t expectedResType);

            // send Other Command + await ACK
            std::error_code sendOtherCommand(const navigator::drivers::seer::SeerNavigatorFrame& req, uint16_t expectedResType);

            // ── JSON helpers ──────────────────────────────────────────────────────────
            static int parseRetCode(const std::string& json);
            static std::string extractTaskId(const std::string& json);
            static SeerStatusAll1 parseAll1Json(const std::string& json);


            SeerNavigatorDriverConfigParams configParams_;

            std::unique_ptr<ports::ISeerNavigatorConnection> statusConn_;   // port 19204 — poll only
            std::unique_ptr<ports::ISeerNavigatorConnection> navConn_;      // port 19206 — nav commands
            std::unique_ptr<ports::ISeerNavigatorConnection> controlConn_;  // port 19205 - control port
            std::unique_ptr<ports::ISeerNavigatorConnection> otherConn_;    // port 19210 - other port

            SeerNavigatorCommandBuilder cmdBuilder_;
            SeerNavigatorStateMapper stateMapper_;

            domain::entities::NavigatorState state_;
            mutable std::mutex mutexState_;

            std::string activeTaskId_; // save id of current task
            PrevSnapshot prevSnapshot_;

            // Event callback (set once before connect, read-only afterwards)
            NavigatorEventCallback eventCallback_;

            // Navigation command serialisation (Q&A mode)
            std::mutex navMutex_;

            // Control command serialisation (Q&A mode)
            std::mutex controlMutex_;

            // Other command serialisation (Q&A mode)
            std::mutex otherMutex_;

            std::thread workerThread_;
            std::atomic<bool> running_{false};
            std::atomic<bool> connected_{false};
            std::atomic<bool> interruptPoll_{false};
    };

}