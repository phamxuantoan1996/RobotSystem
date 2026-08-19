// #include <iostream>
// #include "navigator/domain/value_objects/station.hpp"

// int main(int argc, char *argv[])
// {
//     try 
//     {
//         navigator::domain::value_objects::Station station1("LM123");
//         std::cout << "Station Id : " << station1.getId() << std::endl;

//         navigator::domain::value_objects::Station station2(station1);
//         std::cout << "Station Id : " << station2.getId() << std::endl;

//         navigator::domain::value_objects::Station station3("LM12");

//         if(station1 == station3)
//         {
//             std::cout << "match\n";
//         }
//         else {
//             std::cout << "don't match\n";
//         }
//     } 
//     catch (const std::invalid_argument& e) 
//     {
//         // Nếu constructor quăng ra lỗi, code sẽ nhảy ngay lập tức vào đây
//         std::cerr << e.what() << "\n";
//         return 1;
//     }

//     return 0;
// }


// #include "NavigatorController.cpp"
// #include "navigator/drivers/seer/SeerNavigatorDriverReal.hpp"
// #include "station.hpp"
// #include <csignal>
// #include <iostream>
// #include <memory>

// std::atomic<bool> running{true};
// void signalHandler(int signum) {
//     std::cout << "\nTerminate program...\n" << std::endl;
//     running = false;
// }

// int main(int argc,char *argv[])
// {
//     signal(SIGINT, signalHandler);
//     signal(SIGTERM, signalHandler);
//     auto seerDriver = std::make_unique<navigator::drivers::seer::SeerNavigatorDriverReal>(navigator::drivers::seer::SeerNavigatorDriverConfigParams{
//         .host = "127.0.0.1",
//         .timeout = 3000,
//         .pollStatusIntervals = 100
//     });

//     auto ec = seerDriver->connect();
//     if(ec)
//     {
//         std::cerr << "Can't connect to SEER driver!\n";
//         return 1;
//     }

//     std::cout << "Connected to SEER driver!\n";
//     // std::this_thread::sleep_for(std::chrono::milliseconds(5000));

//     auto navigatorController = std::make_shared<navigator::application::adapter::NavigatorController>(std::move(seerDriver));
//     ec = navigatorController->connect();
//     if(ec)
//     {
//         std::cerr << "Can't intilize Navigator controller!";
//         return 1;
//     }
//     std::cout << "Navigator controller initlized!\n";


//     navigatorController->goToStation(navigator::domain::value_objects::Station("LM16"));
//     // navigatorController->goToPoint(
//     //     navigator::domain::value_objects::Location(0,0,-0.785), 
//     //     navigator::domain::entities::NavigatorBackMode::Forward, 
//     //     navigator::domain::entities::NavigatorCoordinate::SELF);

//     while (running) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     }

//     return 0;
// }

// #include <csignal>
// #include <iostream>
// #include <memory>
// #include <utility>
// #include "gateway/drivers/rest/RestGateway.hpp"
// #include "gateway/application/adapter/GatewayController.hpp"
// #include "navigator/drivers/seer/SeerNavigatorDriverReal.hpp"
// #include "navigator/application/adapter/NavigatorController.hpp"
// #include "robot/application/controller/RobotController.hpp"

// std::atomic<bool> running{true};
// void signalHandler(int signum) {
//     std::cout << "\nTerminate program...\n" << std::endl;
//     running = false;
// }

// int main(int argc,char *argv[])
// {
//     signal(SIGINT, signalHandler);
//     signal(SIGTERM, signalHandler);

//     auto gatewayDriver = std::make_unique<gateway::drivers::rest::RestGateway>("http://127.0.0.1",3000);
//     auto gatewayController = std::make_unique<gateway::application::adapter::GatewayController>(std::move(gatewayDriver));
//     gatewayController->start();

//     auto seerDriver = std::make_unique<navigator::drivers::seer::SeerNavigatorDriverReal>(navigator::drivers::seer::SeerNavigatorDriverConfigParams{
//         .host = "127.0.0.1",
//         .timeout = 3000,
//         .pollStatusIntervals = 100
//     });
//     auto ec = seerDriver->connect();
//     if(ec)
//     {
//         std::cerr << "Can't connect to SEER driver!\n";
//         return 1;
//     }
//     std::cout << "Connected to SEER driver!\n";
//     // std::this_thread::sleep_for(std::chrono::milliseconds(5000));

//     auto navigatorController = std::make_shared<navigator::application::adapter::NavigatorController>(std::move(seerDriver));
//     ec = navigatorController->connect();
//     if(ec)
//     {
//         std::cerr << "Can't intilize Navigator controller!";
//         return 1;
//     }
//     std::cout << "Navigator controller initlized!\n";


//     robot::application::RobotController robotController(navigatorController,std::move(gatewayController));
//     robotController.start();
    
//     std::cout << "exit\n";
//     return 0;
// }


#include <csignal>
#include <iostream>
#include <atomic>
#include <memory>
#include <utility>

#include "../board/application/adapter/BoardController.hpp"
#include "../board/ports/IBoardTransport.hpp"
#include "../board/drivers/serial_transport/BoardSerialTransport.hpp"
#include "../board/domain/value_objects/BoardCommandQueue.hpp"

#include "../lift/application/adapter/LiftController.hpp"
#include "../lift/application/use_cases/LiftMoveStep.hpp"
#include "LiftTarget.hpp"
#include "ports/IRobotStep.hpp"

std::atomic<bool> running{true};
void signalHandler(int signum) {
    std::cout << "\nTerminate program...\n" << std::endl;
    running = false;
}

int main(int argc,char *argv[])
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    board::drivers::serial_transport::SerialTransportConfig serialTransportConfig;
    serialTransportConfig.serial_port = "/dev/ttyUSB0";
    serialTransportConfig.baudrate = 115200;
    serialTransportConfig.timeout = 3000;
    auto boardDriver = std::make_unique<board::drivers::serial_transport::BoardSerialTransport>(std::move(serialTransportConfig));
    auto boardCommandQueue = std::make_shared<board::domain::value_objects::BoardCommandQueue>();
    auto boardController = std::make_shared<board_subsystem::adapter::BoardController>(std::move(boardDriver),boardCommandQueue,100);


    auto liftController = std::make_shared<lift::application::adapter::LiftController>(boardCommandQueue);
    boardController->setCallbackUpdateState([liftController](const std::string& data) {
        liftController->updateState(data);
    });
    auto ec = boardController->connect();

    if(ec)
    {
        std::cerr << "Board Controller Initlize Error\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    while(running)
    {
        lift::application::use_cases::LiftMoveStep step1(liftController,lift::domain::value_objects::LiftTarget(0),0);
        step1.excute(common::ports::UnknowStepResult{});
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        lift::application::use_cases::LiftMoveStep step2(liftController,lift::domain::value_objects::LiftTarget(1),1);
        step2.excute(common::ports::UnknowStepResult{});
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
    return 0;
}