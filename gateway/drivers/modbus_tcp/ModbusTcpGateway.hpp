#pragma once
#include "../gateway/ports/IRobotGateway.hpp"
#include <atomic>
#include <modbus/modbus.h>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace gateway::drivers::modbus_tcp {

    enum class HoldingRegisterAddress {
    // Relocation params
        CoordinateX = 1,
        CoordinateY = 3,
        CoordinateAngular = 5,
        // Switch map param
        MapId = 7,
        // Select shelf param
        ShelfId = 8,
        // Open loop motion
        VelocityX = 10,
        VelocityY = 12,
        VelocityW = 14,
        Duration = 16,
        // Task params
        TaskTypeReq = 20,
        TaskIdReq = 21,
        TargetReq = 22
    };

    enum class CoilAddress {
        Pause = 1,
        Resume = 2,
        Cancel = 3,
        Start = 4,
        ClearError = 5,
        SwitchMap = 6,
        SetShelf = 7,
        ClearShelf = 8,
        OpenLoopMotion = 9,
        Relocation = 10
    };

    enum class InputRegisterAddress {
        // navigation
        CoordinateX = 1,
        CoordinateY = 3,
        CoordinateAngular = 5,
        LocationState = 7,
        Confidence = 8,
        BatteryLevel = 10, // 0 -> 100
        BatteryTemp = 12,
        BatteryVol = 14,
        BatteryCurrent = 16,
        CurrentStation = 18,
        NextStation = 19,
        VelocityX = 20,
        VelocityY = 22,
        VelocityAngular = 24,
        MapId = 26,
        NavigationError = 27,

        // lift
        LiftPosition = 28,
        LiftError = 29,

        RobotStatus = 30,

        // mission 
        MissionStatus = 31,
        MissionId = 32,
        
        // total distance
        TotalDistance = 33,

        // time
        Time = 35,

        // Shelf Id
        ShelfId = 37,

        //
        NavigatorIp = 40,
        TaskTypeRes = 44,
        TaskIdRes = 45,
        TargetRes = 46
    };

    enum class DiscreteInputAddress {
        Block = 2,
        Estop = 3,
        Charing = 4,
    };

    struct SignalControl {
        uint8_t pause = 0;
        uint8_t resume = 0;
        uint8_t cancel = 0;
        uint8_t start = 0;
        uint8_t switch_map = 0;
        uint8_t set_shelf = 0;
        uint8_t clear_shelf = 0;
        uint8_t clear_error = 0;
        uint8_t relocation = 0;
    }; 


    class ModbusTcpGateway : public gateway::ports::IRobotGateway {
        public:
            ModbusTcpGateway(std::string host, uint16_t port);
            ~ModbusTcpGateway();

            // sendStatus
            gateway::domain::entities::NetworkResult sendStatus(const std::string& payload) override;
            
            // sendRequest
            gateway::domain::entities::NetworkResult sendRequest(const std::string& payload) override;

            // sendReponse
            gateway::domain::entities::NetworkResult sendResponse(const std::string& payload) override;

            // start
            void start() override;

            // stop
            void stop() override;

            void setGatewayEventCallback(GatewayEventCallback cb) override;

            void setGatewayGetRobotCallback(GatewayGetRobotStateCallback cb) override;
        
        private:
            std::atomic<bool> running_{false};
            SignalControl signalControlCache_;
            std::thread workerLoopThread_;
            std::thread acceptClientThread_;

            std::vector<std::thread> clientThreads_;
            std::mutex threadsMutex_;

            std::mutex mutexMapping_;
            modbus_mapping_t *mbMapping_ = nullptr;
            std::atomic<int> activeClients_{0};
            std::atomic<int> clientCount_{0};
            modbus_t *ctx_;
            std::string host_;
            uint16_t port_;
            int serverSocket_ = -1;

            static constexpr int NUM_OF_HOLDING_REGS = 30;
            static constexpr int NUM_OF_INPUT_REGS = 50;
            static constexpr int NUM_OF_COILS = 20;
            static constexpr int NUM_OF_DECRETE_INPUT = 20;

            bool isSocketAlive(int sock);
            void clientHandler(int client_socket);
            void initializeModbusData();
            void workerLoopThreadEntry(void);
            void acceptClientThreadEntry(void);

            GatewayEventCallback eventCallback_;
            GatewayGetRobotStateCallback getRobotStatusCallback_;
    };
}