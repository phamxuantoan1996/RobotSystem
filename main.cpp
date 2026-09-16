// // modbus
// // test ghep noi cac module
// #include "ModbusTcpGateway.hpp"
// #include "navigator/application/adapter/NavigatorController.hpp"
// #include "navigator/drivers/seer/SeerNavigatorDriverReal.hpp"

// #include "board/application/adapter/BoardController.hpp"
// #include "board/drivers/serial_transport/BoardSerialTransport.hpp"
// #include "board/domain/value_objects/BoardCommandQueue.hpp"

// #include "lift/application/adapter/LiftController.hpp"

// #include "indicator/application/adapter/IndicatorController.hpp"

// #include "gateway/application/adapter/GatewayController.hpp"

// #include "reactor/indicator/IndicatorReactor.hpp"
// #include "robot/application/controller/RobotController.hpp"

// #include "logger/application/adapter/LogController.hpp"
// #include "logger/drivers/file/FileLogWriter.hpp"

// #include <jsoncpp/json/json.h>
// #include <csignal>
// #include <iostream>
// #include <atomic>
// #include <jsoncpp/json/value.h>
// #include <memory>
// #include <utility>

// std::atomic<bool> running{true};
// void signalHandler(int signum) {
//     std::cout << "\nTerminate program...\n" << std::endl;
//     running = false;
// }

// int main(int argc, char *argv[])
// {
//     signal(SIGINT, signalHandler);
//     signal(SIGTERM, signalHandler);

//     // load config params
//     std::ifstream config_file("config.json", std::ifstream::binary);
//     if (!config_file.is_open()) {
//         std::cerr << "Error: Can't open config file." << std::endl;
//         return 1; // Thoát chương trình nếu không có cấu hình
//     }

//     Json::Value config;
//     Json::CharReaderBuilder reader_builder;
//     std::string errs;
//     if (!Json::parseFromStream(reader_builder, config_file, &config, &errs)) {
//         std::cerr << "Error: Systax config.json: " << errs << std::endl;
//         return 1;
//     }
//     // Đóng file sau khi đọc xong để giải phóng tài nguyên
//     config_file.close();

//     if(!config.isMember("gateway") || !config["gateway"].isObject()
//         || !config.isMember("navigator") || !config["navigator"].isObject()
//         || !config.isMember("board") || !config["board"].isObject())
//     {
//         return 1;
//     }

//     const Json::Value& board_config = config["board"];
//     const Json::Value& navigator_config = config["navigator"];
//     const Json::Value& gateway_config = config["gateway"];

//     // khoi tao lift, indicator, board
//     if(!board_config.isMember("type") && !board_config.isString())
//     {
//         return 1;
//     }
//     if(board_config["type"].asString() != "serial" || !board_config.isMember("params") || !board_config["params"].isObject())
//     {
//         return 1;
//     }
//     const Json::Value& board_config_params = board_config["params"];
//     if(!board_config_params.isMember("port") || !board_config_params["port"].isString()
//         || !board_config_params.isMember("baudrate") || !board_config_params["baudrate"].isInt())
//     {
//         return 1;
//     }
//     board::drivers::serial_transport::SerialTransportConfig serialTransportConfig;
//     serialTransportConfig.serial_port = board_config_params["port"].asString();
//     serialTransportConfig.baudrate = board_config_params["baudrate"].asInt();
//     serialTransportConfig.timeout = 3000;
//     auto boardDriver = std::make_unique<board::drivers::serial_transport::BoardSerialTransport>(std::move(serialTransportConfig));
//     auto boardCommandQueue = std::make_shared<board::domain::value_objects::BoardCommandQueue>();
//     auto boardController = std::make_shared<board::application::adapter::BoardController>(std::move(boardDriver),boardCommandQueue,100);


//     auto liftController = std::make_shared<lift::application::adapter::LiftController>(boardCommandQueue);
//     boardController->setCallbackUpdateState([liftController](const std::string& data) {
//         liftController->updateState(data);
//     });

//     auto indicatorController = std::make_unique<indicator::application::adapter::IndicatorController>(boardCommandQueue);
//     auto ec = boardController->connect();
//     if(ec)
//     {
//         std::cerr << "Board Controller Initlize Error\n";
//         return 1;
//     }

//     // khoi tao navigator controller
//     if(!navigator_config.isMember("ip_address") || !navigator_config["ip_address"].isString())
//     {
//         return 1;
//     }
//     auto seerDriver = std::make_unique<navigator::drivers::seer::SeerNavigatorDriverReal>(navigator::drivers::seer::SeerNavigatorDriverConfigParams{
//         .host = navigator_config["ip_address"].asString(),
//         .timeout = 3000,
//         .pollStatusIntervals = 100
//     });
//     ec = seerDriver->connect();
//     if(ec)
//     {
//         std::cerr << "Can't connect to SEER driver!\n";
//         return 1;
//     }
//     std::cout << "Connected to SEER driver!\n";

    

//     auto navigatorController = std::make_shared<navigator::application::adapter::NavigatorController>(std::move(seerDriver));
//     navigatorController->onRelocation([&navigatorController](const navigator::domain::events::NavigatorRelocationConfirmEvent&) {
//         std::cout << "confirm location\n";
//         navigatorController->confirmLocation();
//     });
//     ec = navigatorController->connect();
//     if(ec)
//     {
//         std::cerr << "Can't intilize Navigator controller!";
//         return 1;
//     }
//     std::cout << "Navigator controller initlized!\n";

//     // khoi tao gateway api
//     // auto gatewayDriver = std::make_unique<gateway::drivers::rest::RestGateway>("http://127.0.0.1",3000);
//     if(!gateway_config.isMember("type") || !gateway_config["type"].isString() || !gateway_config.isMember("params") || !gateway_config["params"].isObject())
//     {
//         return 1;
//     }
//     if(gateway_config["type"].asString() != "modbus")
//     {
//         return 1;
//     }
//     const Json::Value& gateway_config_params = gateway_config["params"];
//     if(!gateway_config_params.isMember("ip_address") || !gateway_config_params["ip_address"].isString()
//         || !gateway_config_params.isMember("port") || !gateway_config_params["port"].isInt())
//     {
//         return 1;
//     }

//     std::unique_ptr<gateway::drivers::modbus_tcp::ModbusTcpGateway> gatewayDriver;
//     try {
//         gatewayDriver = std::make_unique<gateway::drivers::modbus_tcp::ModbusTcpGateway>(gateway_config_params["ip_address"].asString(),gateway_config_params["port"].asInt());
//     } catch (const std::exception& e) {
//         return 1;
//     }
//     auto gatewayController = std::make_shared<gateway::application::adapter::GatewayController>(std::move(gatewayDriver));
//     gatewayController->start();


//     // khoi tao robot controller
//     auto robotController = std::make_shared<robot::application::RobotController>(boardController,navigatorController,gatewayController,liftController);
    
//     auto indicatorReactor = std::make_unique<reactor::IndicatorReactor>(robotController,boardController,navigatorController,std::move(indicatorController),liftController);
//     indicatorReactor->start();

//     auto loggerDriver = std::make_unique<logger::drivers::file::FileLogWriter>();
//     auto loggerController = std::make_unique<logger::application::adapter::LogController>(std::move(loggerDriver),robotController,boardController,navigatorController,liftController);

//     robotController->start();

//     // khoi tao indicator reactor
//     while (running) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     }
    
//     return 0;
// }






// // rest api
// // test ghep noi cac module
// #include "navigator/application/adapter/NavigatorController.hpp"
// #include "navigator/drivers/seer/SeerNavigatorDriverReal.hpp"
// #include "board/application/adapter/BoardController.hpp"
// #include "board/drivers/serial_transport/BoardSerialTransport.hpp"
// #include "board/domain/value_objects/BoardCommandQueue.hpp"
// #include "lift/application/adapter/LiftController.hpp"
// #include "indicator/application/adapter/IndicatorController.hpp"
// #include "gateway/application/adapter/GatewayController.hpp"
// #include "gateway/drivers/rest/RestGateway.hpp"
// #include "reactor/indicator/IndicatorReactor.hpp"
// #include "robot/application/controller/RobotController.hpp"
// #include "logger/application/adapter/LogController.hpp"
// #include "logger/drivers/file/FileLogWriter.hpp"

// #include <jsoncpp/json/json.h>
// #include <csignal>
// #include <iostream>
// #include <atomic>
// #include <jsoncpp/json/value.h>
// #include <memory>
// #include <utility>

// std::atomic<bool> running{true};
// void signalHandler(int signum) {
//     std::cout << "\nTerminate program...\n" << std::endl;
//     running = false;
// }

// int main(int argc, char *argv[])
// {
//     signal(SIGINT, signalHandler);
//     signal(SIGTERM, signalHandler);

//     // load config params
//     std::ifstream config_file("config.json", std::ifstream::binary);
//     if (!config_file.is_open()) {
//         std::cerr << "Error: Can't open config file." << std::endl;
//         return 1; // Thoát chương trình nếu không có cấu hình
//     }

//     Json::Value config;
//     Json::CharReaderBuilder reader_builder;
//     std::string errs;
//     if (!Json::parseFromStream(reader_builder, config_file, &config, &errs)) {
//         std::cerr << "Error: Systax config.json: " << errs << std::endl;
//         return 1;
//     }
//     // Đóng file sau khi đọc xong để giải phóng tài nguyên
//     config_file.close();

//     if(!config.isMember("gateway") || !config["gateway"].isObject()
//         || !config.isMember("navigator") || !config["navigator"].isObject()
//         || !config.isMember("board") || !config["board"].isObject())
//     {
//         return 1;
//     }

//     const Json::Value& board_config = config["board"];
//     const Json::Value& navigator_config = config["navigator"];
//     const Json::Value& gateway_config = config["gateway"];

//     // khoi tao lift, indicator, board
//     if(!board_config.isMember("type") && !board_config.isString())
//     {
//         return 1;
//     }
//     if(board_config["type"].asString() != "serial" || !board_config.isMember("params") || !board_config["params"].isObject())
//     {
//         return 1;
//     }
//     const Json::Value& board_config_params = board_config["params"];
//     if(!board_config_params.isMember("port") || !board_config_params["port"].isString()
//         || !board_config_params.isMember("baudrate") || !board_config_params["baudrate"].isInt())
//     {
//         return 1;
//     }
//     board::drivers::serial_transport::SerialTransportConfig serialTransportConfig;
//     serialTransportConfig.serial_port = board_config_params["port"].asString();
//     serialTransportConfig.baudrate = board_config_params["baudrate"].asInt();
//     serialTransportConfig.timeout = 3000;
//     auto boardDriver = std::make_unique<board::drivers::serial_transport::BoardSerialTransport>(std::move(serialTransportConfig));
//     auto boardCommandQueue = std::make_shared<board::domain::value_objects::BoardCommandQueue>();
//     auto boardController = std::make_shared<board::application::adapter::BoardController>(std::move(boardDriver),boardCommandQueue,100);


//     auto liftController = std::make_shared<lift::application::adapter::LiftController>(boardCommandQueue);
//     boardController->setCallbackUpdateState([liftController](const std::string& data) {
//         liftController->updateState(data);
//     });

//     auto indicatorController = std::make_unique<indicator::application::adapter::IndicatorController>(boardCommandQueue);
//     auto ec = boardController->connect();
//     if(ec)
//     {
//         std::cerr << "Board Controller Initlize Error\n";
//         return 1;
//     }

//     // khoi tao navigator controller
//     if(!navigator_config.isMember("ip_address") || !navigator_config["ip_address"].isString())
//     {
//         return 1;
//     }
//     auto seerDriver = std::make_unique<navigator::drivers::seer::SeerNavigatorDriverReal>(navigator::drivers::seer::SeerNavigatorDriverConfigParams{
//         .host = navigator_config["ip_address"].asString(),
//         .timeout = 3000,
//         .pollStatusIntervals = 100
//     });
//     ec = seerDriver->connect();
//     if(ec)
//     {
//         std::cerr << "Can't connect to SEER driver!\n";
//         return 1;
//     }
//     std::cout << "Connected to SEER driver!\n";

    

//     auto navigatorController = std::make_shared<navigator::application::adapter::NavigatorController>(std::move(seerDriver));
//     navigatorController->onRelocation([&navigatorController](const navigator::domain::events::NavigatorRelocationConfirmEvent&) {
//         std::cout << "confirm location\n";
//         navigatorController->confirmLocation();
//     });
//     ec = navigatorController->connect();
//     if(ec)
//     {
//         std::cerr << "Can't intilize Navigator controller!";
//         return 1;
//     }   
//     std::cout << "Navigator controller initlized!\n";

//     // khoi tao gateway api
//     auto gatewayDriver = std::make_unique<gateway::drivers::rest::RestGateway>("http://10.10.100.11:9000",3000);
//     auto gatewayController = std::make_shared<gateway::application::adapter::GatewayController>(std::move(gatewayDriver));
//     gatewayController->start();


//     // khoi tao robot controller
//     auto robotController = std::make_shared<robot::application::RobotController>(boardController,navigatorController,gatewayController,liftController);
    
//     auto indicatorReactor = std::make_unique<reactor::IndicatorReactor>(robotController,boardController,navigatorController,std::move(indicatorController),liftController);
//     indicatorReactor->start();

//     auto loggerDriver = std::make_unique<logger::drivers::file::FileLogWriter>();
//     auto loggerController = std::make_unique<logger::application::adapter::LogController>(std::move(loggerDriver),robotController,boardController,navigatorController,liftController);

//     robotController->start();
//     // khoi tao indicator reactor
//     while (running) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     }
    
//     return 0;
// }






#include "BoardController.hpp"
#include "JoyStickEvent.hpp"
#include "board/drivers/tcp_transport/BoardTcpTransport.hpp"
#include "board/domain/value_objects/BoardCommandQueue.hpp"
#include "joystick/application/adapter/JoyStickController.hpp"
#include "joystick/drivers/JoyStickPLC/JoyStickPLC.hpp"
#include <csignal>
#include <memory>
#include <string>
std::atomic<bool> running{true};
void signalHandler(int signum) {
    std::cout << "\nTerminate program...\n" << std::endl;
    running = false;
}

int main(int argc,char *argv[])
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    auto joystickDriver = std::make_shared<joystick::drivers::plc::JoyStickPLC>();
    auto joyStickController = std::make_shared<joystick::application::adapters::JoyStickController>(joystickDriver);
    joyStickController->subscribeEvents([](const joystick::domain::events::JoyStickEvent& event){
        std::visit([](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, joystick::domain::events::JoyStickSetTurnLeftEvent>)
            {
                std::cout << "set turn left\n";
            }
            else if constexpr (std::is_same_v<T, joystick::domain::events::JoyStickClearTurnLeftEvent>)
            {
                std::cout << "clear turn left\n";
            }
            if constexpr (std::is_same_v<T, joystick::domain::events::JoyStickSetTurnRightEvent>)
            {
                std::cout << "set turn right\n";
            }
            else if constexpr (std::is_same_v<T, joystick::domain::events::JoyStickClearTurnRightEvent>)
            {
                std::cout << "clear turn right\n";
            }

        },event);
    });
    

    auto boardDriver = std::make_unique<board::drivers::tcp_transport::BoardTcpTransport>(board::drivers::tcp_transport::TcpTransportConfig{
        .host = "10.10.100.200",
        .port = 5000,
        .timeout = 3000});

    auto ec = boardDriver->connect();
    if(ec)
    {
        std::cout << "1\n";
        return 1;
    }

    auto boardCommandQueue = std::make_shared<board::domain::value_objects::BoardCommandQueue>();
    auto boardController = std::make_shared<board::application::adapter::BoardController>(std::move(boardDriver),boardCommandQueue,100);
    boardController->setCallbackUpdateState([joystickDriver](const std::string& raw_state){
        joystickDriver->updateState(raw_state);
    });

    ec = boardController->connect();
    if(ec)
    {
        return 1;
    }

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    return 0;
}
