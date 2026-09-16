#include "../gateway/drivers/modbus_tcp/ModbusTcpGateway.hpp"
#include "GatewayEvent.hpp"
#include <jsoncpp/json/json.h>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <json/value.h>
#include <modbus/modbus.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <vector>

namespace gateway::drivers::modbus_tcp {
    ModbusTcpGateway::ModbusTcpGateway(std::string host, uint16_t port) : host_(host), port_(port)
    {
        modbus_t *ctx = modbus_new_tcp(host.c_str(), port);
        if (ctx == nullptr) {
            throw std::runtime_error("Failed to create Modbus TCP context: " + std::string(modbus_strerror(errno)));
        }
        ctx_ = ctx;

        mbMapping_ = modbus_mapping_new(NUM_OF_COILS, NUM_OF_DECRETE_INPUT, NUM_OF_HOLDING_REGS, NUM_OF_INPUT_REGS);
        if (mbMapping_ == nullptr) {
            std::cerr << "[Lỗi] Không thể tạo Modbus mapping" << std::endl;
            modbus_free(ctx);
            throw std::runtime_error("Failed to create modbus mapping: " + std::string(modbus_strerror(errno)));
        }

        initializeModbusData();

        // Thiết lập socket server
        serverSocket_ = modbus_tcp_listen(ctx, 10); // backlog = 10
        if (serverSocket_ == -1) {
            std::cerr << "[Lỗi] Không thể lắng nghe trên cổng 1502" << std::endl;
            std::cout << "Gặp loi listen: " << modbus_strerror(errno) << std::endl; 
            modbus_mapping_free(mbMapping_);
            modbus_free(ctx);
            throw std::runtime_error("Can't listen on port: " + std::string(modbus_strerror(errno)));
        }

        // Set timeout cho server socket
        struct timeval server_timeout;
        server_timeout.tv_sec = 1;
        server_timeout.tv_usec = 0;
        setsockopt(serverSocket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&server_timeout, sizeof(server_timeout));

        std::cout << "[Hệ thống] Modbus TCP Server đang lắng nghe tại cổng " << port << " ...." << std::endl;
        std::cout << "==========================" << std::endl;
    }

    ModbusTcpGateway::~ModbusTcpGateway()
    {
        stop();
    }

    void ModbusTcpGateway::start()
    {
        bool expected = true;
        if(running_.compare_exchange_strong(expected,true))
        {
            return;
        }
        running_ = true;
        std::cout << "accept client thread\n";
        acceptClientThread_ = std::thread(&ModbusTcpGateway::acceptClientThreadEntry,this);
        workerLoopThread_ = std::thread(&ModbusTcpGateway::workerLoopThreadEntry,this);

    }

    void ModbusTcpGateway::stop()
    {
        if(!running_)
            return;
        running_ = false;
        if(serverSocket_ != -1)
        {
            close(serverSocket_);
            serverSocket_ = -1;
        }
            

        std::cout << "[Hệ thống] Đang đợi tất cả client thread kết thúc..." << std::endl;

        // Chờ và giải phóng tất cả các thread client
        {
            std::lock_guard<std::mutex> lock(threadsMutex_);
            for (auto& t : clientThreads_) {
                if (t.joinable()) {
                    t.join(); // Đợi thread dọn dẹp xong dữ liệu rồi mới tắt hẳn
                }
            }
            clientThreads_.clear(); // Xóa sạch danh sách
            std::cout << "[Hệ thống] Gateway đã dừng hoàn toàn an toàn." << std::endl;
        }

        // 3. GIẢI PHÓNG VÙNG NHỚ MAPPING (An toàn vì không còn thread nào sử dụng nữa)
        {
            std::lock_guard<std::mutex> lock(mutexMapping_);
            if (mbMapping_ != nullptr) {
                modbus_mapping_free(mbMapping_);
                mbMapping_ = nullptr;
                std::cout << "[Hệ thống] Đã giải phóng Modbus Mapping." << std::endl;
            }
        }

        // 4. GIẢI PHÓNG SERVER CONTEXT CHÍNH
        if (ctx_ != nullptr) {
            modbus_free(ctx_);
            ctx_ = nullptr;
            std::cout << "[Hệ thống] Đã giải phóng Server Modbus Context." << std::endl;
        }
    }

    bool ModbusTcpGateway::isSocketAlive(int sock)
    {
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
            return error == 0;
        }
        return false;
    }
    void ModbusTcpGateway::clientHandler(int client_socket)
    {
        activeClients_.fetch_add(1);

        // Lấy thông tin client
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);
        std::string client_ip = "Unknown";
        int client_port = 0;

        if (getpeername(client_socket, (struct sockaddr *)&addr, &addr_size) == 0) {
            char ip_str[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN) != nullptr) {
                client_ip = ip_str;
                client_port = ntohs(addr.sin_port);
            }
        }

        // std::cout << "[Hệ thống] [Luồng " << std::this_thread::get_id() 
        //         << "] Đang phục vụ Client từ IP: " << client_ip 
        //         << ":" << client_port << std::endl;

        // 1. Tạo context và gắn socket
        modbus_t *client_ctx = modbus_new_tcp(nullptr, 0);
        if (client_ctx == nullptr) {
            std::cerr << "[Lỗi] Không thể tạo modbus context" << std::endl;
            close(client_socket);
            activeClients_.fetch_sub(1);
            return;
        }

        if (modbus_set_socket(client_ctx, client_socket) == -1) {
            std::cerr << "[Lỗi] Không thể gắn socket vào modbus context" << std::endl;
            modbus_free(client_ctx);
            close(client_socket);
            activeClients_.fetch_sub(1);
            return;
        }

        // 2. QUAN TRỌNG: Set timeout cho libmodbus (SỬA ĐÚNG CÚ PHÁP)
        // modbus_set_response_timeout(ctx, sec, usec) - 3 tham số
        modbus_set_response_timeout(client_ctx, 5, 0);  // 5 giây timeout
        modbus_set_byte_timeout(client_ctx, 1, 0);      // 1 giây byte timeout

        // 3. Set timeout cho socket (backup)
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

        // 4. Đặt socket ở chế độ non-blocking
        int flags = fcntl(client_socket, F_GETFL, 0);
        if (flags == -1) {
            std::cerr << "[Lỗi] Không thể lấy flags của socket" << std::endl;
            modbus_free(client_ctx);
            close(client_socket);
            activeClients_.fetch_sub(1);
            return;
        }
        
        if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
            std::cerr << "[Lỗi] Không thể set non-blocking cho socket" << std::endl;
            modbus_free(client_ctx);
            close(client_socket);
            activeClients_.fetch_sub(1);
            return;
        }

        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;
        int consecutive_errors = 0;
        const int MAX_CONSECUTIVE_ERRORS = 3;

        while (running_.load() && activeClients_.load() > 0) {
            // Sử dụng select để kiểm tra dữ liệu
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(client_socket, &readfds);
            
            struct timeval select_timeout;
            select_timeout.tv_sec = 1;  // Check mỗi 1 giây
            select_timeout.tv_usec = 0;

            int select_rc = select(client_socket + 1, &readfds, NULL, NULL, &select_timeout);

            if (select_rc < 0) {
                if (errno == EINTR) continue; // Bị interrupt bởi signal
                std::cerr << "[Lỗi select] " << strerror(errno) << std::endl;
                break;
            } 
            else if (select_rc == 0) {
                // Timeout - Kiểm tra kết nối vẫn còn sống
                if (!isSocketAlive(client_socket)) {
                    std::cout << "[Kết nối] Socket đã đóng hoặc lỗi" << std::endl;
                    break;
                }
                
                // Kiểm tra nếu thread đang bị treo quá lâu
                consecutive_errors++;
                if (consecutive_errors >= 5) { // 30 giây không có dữ liệu
                    std::cout << "[Kết nối] Không có dữ liệu trong 30 giây, ngắt kết nối" << std::endl;
                    break;
                }
                continue;
            }

            // Có dữ liệu để đọc
            if (FD_ISSET(client_socket, &readfds)) {
                consecutive_errors = 0; // Reset counter
                
                // Đọc dữ liệu Modbus
                rc = modbus_receive(client_ctx, query);
                
                if (rc > 0) {
                    // Xử lý request với mutex
                    {
                        std::lock_guard<std::mutex> lock(mutexMapping_);
                        if (mbMapping_ != nullptr) {
                            modbus_reply(client_ctx, query, rc, mbMapping_);
                        }
                    }
                } 
                else if (rc == -1) {
                    // Kiểm tra lỗi cụ thể
                    int err = errno;
                    if (err == EAGAIN || err == EWOULDBLOCK) {
                        // Không có dữ liệu, tiếp tục
                        continue;
                    }
                    // std::cout << "[Modbus error] " << modbus_strerror(err) << " (errno: " << err << ")" << std::endl;
                    
                    // Nếu là lỗi timeout hoặc connection reset thì break
                    if (err == ETIMEDOUT || err == ECONNRESET || err == EPIPE) {
                        break;
                    }
                    
                    // Tăng counter cho các lỗi khác
                    consecutive_errors++;
                    if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
                        std::cout << "[Kết nối] Quá nhiều lỗi liên tiếp, ngắt kết nối" << std::endl;
                        break;
                    }
                } 
                else {
                    // rc == 0: Connection closed
                    std::cout << "[Kết nối] Client đã đóng kết nối" << std::endl;
                    break;
                }
            }
        }

        // Cleanup
        // std::cout << "[Hệ thống] Thread cho client " << client_ip << ":" << client_port << " kết thúc" << std::endl;
        
        // Đóng socket và giải phóng context
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        modbus_free(client_ctx);
        activeClients_.fetch_sub(1);
    }
    void ModbusTcpGateway::initializeModbusData()
    {
        if (mbMapping_ == nullptr) return;
    
        // 1. Coils (mảng uint8_t, giá trị 0 hoặc 1)
        for (int i = 0; i < NUM_OF_COILS; i++) {
            mbMapping_->tab_bits[i] = 0;
        }
        
        // 2. Discrete Inputs (mảng uint8_t chỉ đọc)
        for (int i = 0; i < NUM_OF_DECRETE_INPUT; i++) {
            mbMapping_->tab_input_bits[i] = 0;
        }
        
        // 3. Holding Registers (mảng uint16_t đọc/ghi)
        for (int i = 0; i < NUM_OF_HOLDING_REGS; i++) {
            mbMapping_->tab_registers[i] = 0;
        }
        
        // 4. Input Registers (mảng uint16_t chỉ đọc)
        for (int i = 0; i < NUM_OF_INPUT_REGS; i++) {
            mbMapping_->tab_input_registers[i] = 0;
        }
    }
    void ModbusTcpGateway::workerLoopThreadEntry(void)
    {
        while (running_) {
            {
                std::lock_guard<std::mutex> lk(mutexMapping_);

                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::Pause)] == 1 && signalControlCache_.pause == 0)
                {
                    signalControlCache_.pause = 1;
                    if(eventCallback_)
                    {
                        eventCallback_(gateway::domain::events::SignalPauseEvent{});
                    }
                }
                else if (mbMapping_->tab_bits[static_cast<int>(CoilAddress::Pause)] == 0 && signalControlCache_.pause == 1)
                {
                    signalControlCache_.pause = 0;
                }

                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::Resume)] == 1 && signalControlCache_.resume == 0)
                {
                    signalControlCache_.resume = 1;
                    if(eventCallback_)
                    {
                        eventCallback_(gateway::domain::events::SignalResumeEvent{});
                    }
                }
                else if (mbMapping_->tab_bits[static_cast<int>(CoilAddress::Resume)] == 0 && signalControlCache_.resume == 1)
                {
                    signalControlCache_.resume = 0;
                }

                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::Cancel)] == 1 && signalControlCache_.cancel == 0)
                {
                    signalControlCache_.cancel = 1;
                    if(eventCallback_)
                    {
                        eventCallback_(gateway::domain::events::SignalCancelEvent{});
                    }
                }
                else if (mbMapping_->tab_bits[static_cast<int>(CoilAddress::Cancel)] == 0 && signalControlCache_.cancel == 1)
                {
                    signalControlCache_.cancel = 0;
                }

                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::Start)] == 1 && signalControlCache_.start == 0)
                {
                    uint16_t mission_id = mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TaskIdReq)];
                    uint16_t mission_type = mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TaskTypeReq)];
                    uint16_t target = mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TargetReq)];
                    if(mission_id != 0)
                    {
                        std::string mission_gen = "";
                        switch (mission_type) {
                            case 1: // navigate
                            {
                                /*
                                {
                                    "mission_id" = "123456";
                                    "activity_type" = 0;
                                    "action_list" : [
                                        {
                                            "name" : "action_navigation",
                                            "params" : {
                                                "target" : "LM123"
                                            }
                                        }
                                    ]
                                }
                                */
                                if(target == 0)
                                    break;

                                Json::Value root;
                                root["mission_id"] = std::to_string(mission_id);
                                root["activity_type"] = 0;

                                Json::Value navigation_params;
                                navigation_params["target"] = "LM" + std::to_string(target);

                                Json::Value navigation;
                                navigation["name"] = "action_navigation";
                                navigation["params"] = navigation_params;

                                Json::Value array(Json::arrayValue);
                                array.append(navigation);
                                root["action_list"] = array;

                                Json::StreamWriterBuilder builder;
                                builder["commentStyle"] = "None";
                                builder["indentation"] = ""; // Xóa thụt lề
                                mission_gen = Json::writeString(builder, root);

                                break;
                            }
                            case 2: // lift
                            {
                                /*
                                {
                                    "mission_id" = "123456";
                                    "activity_type" = 0;
                                    "action_list" : [
                                        {
                                            "name" : "action_lift",
                                            "params" : {
                                                "target" : 0
                                            }
                                        }
                                    ]
                                }
                                */
                                if(target != 1 && target != 2)
                                {
                                    break;
                                }
                                target = target - 1;
                                Json::Value root;
                                root["mission_id"] = std::to_string(mission_id);
                                root["activity_type"] = 0;

                                Json::Value navigation_params;
                                navigation_params["target"] = target;

                                Json::Value navigation;
                                navigation["name"] = "action_lift";
                                navigation["params"] = navigation_params;

                                Json::Value array(Json::arrayValue);
                                array.append(navigation);
                                root["action_list"] = array;

                                Json::StreamWriterBuilder builder;

                                builder["commentStyle"] = "None";
                                builder["indentation"] = ""; // Xóa thụt lề
                                mission_gen = Json::writeString(builder, root);

                                break;
                            }
                            default:
                            {
                                mission_id = 0;
                                break;
                            }
                        }
                        if(!mission_gen.empty())
                        {
                            signalControlCache_.start = 1;
                            if(eventCallback_)
                            {
                                eventCallback_(gateway::domain::events::MissionDispatchEvent{.mission = mission_gen});
                            }
                        }
                        
                    }
                    mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TaskTypeReq)] = 0;
                    mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TargetReq)] = 0;
                    mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TaskIdReq)] = 0;
                }
                else if (mbMapping_->tab_bits[static_cast<int>(CoilAddress::Start)] == 0 && signalControlCache_.start == 1)
                {
                    signalControlCache_.start = 0;
                }

                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::ClearError)] == 1 && signalControlCache_.clear_error == 0)
                {
                    // clear error
                    signalControlCache_.clear_error = 1;
                    if(eventCallback_)
                    {
                        eventCallback_(gateway::domain::events::SignalClearErrorEvent{});
                    }
                }
                else if (mbMapping_->tab_bits[static_cast<int>(CoilAddress::ClearError)] == 0 && signalControlCache_.clear_error == 1)
                {
                    signalControlCache_.clear_error = 0;
                }
            
                // switch map
                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::SwitchMap)] == 1 && signalControlCache_.switch_map == 0)
                {
                    signalControlCache_.switch_map = 1;
                    std::string map_name = std::to_string(mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::MapId)]);
                    if(eventCallback_)
                    {
                        eventCallback_(gateway::domain::events::SwitchMapEvent{.map_name = map_name});
                    }
                    mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::MapId)] = 0;
                }
                else if (mbMapping_->tab_bits[static_cast<int>(CoilAddress::SwitchMap)] == 0 && signalControlCache_.switch_map == 1)
                {
                    signalControlCache_.switch_map = 0;
                }
            
                // set shelf
                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::SetShelf)] == 1 && signalControlCache_.set_shelf == 0)
                {
                    signalControlCache_.set_shelf = 1;
                    std::string shelf_name = std::to_string(mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::ShelfId)]);
                    if(eventCallback_)
                    {
                        eventCallback_(gateway::domain::events::SetShelfEvent{.shelf_name = "shelf/" + shelf_name + ".shelf"});
                    }
                    mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::ShelfId)] = 0;
                }
                else if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::SetShelf)] == 0 && signalControlCache_.set_shelf == 1) {
                    signalControlCache_.set_shelf = 0;
                }

                // clear shelf
                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::ClearShelf)] == 1 && signalControlCache_.clear_shelf == 0)
                {
                    signalControlCache_.clear_shelf = 1;
                    if(eventCallback_)
                    {
                        eventCallback_(gateway::domain::events::ClearShelfEvent{});
                    }
                }
                else if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::ClearShelf)] == 0 && signalControlCache_.clear_shelf == 1)
                {
                    signalControlCache_.clear_shelf = 0;
                }

                // relocation
                if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::Relocation)] == 1 && signalControlCache_.clear_shelf == 0)
                {
                    signalControlCache_.relocation = 1;
                    if(eventCallback_)
                    {
                        float x = 0;
                        float y = 0;
                        float angle = 0;

                        uint32_t combinedX = ((uint32_t)mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateX)] << 16) | mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateX) + 1];
                        std::memcpy(&x, &combinedX, sizeof(x));

                        uint32_t combinedY = ((uint32_t)mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateY)] << 16) | mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateY) + 1];
                        std::memcpy(&y, &combinedY, sizeof(y));

                        uint32_t combinedAngular = ((uint32_t)mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateAngular)] << 16) | mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateAngular) + 1];
                        std::memcpy(&angle, &combinedAngular, sizeof(angle));

                        eventCallback_(gateway::domain::events::RelocationEvent{.x = x, .y = y, .angle = angle});
                        mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateX)] = 0;
                        mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateX) + 1] = 0;
                        mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateY)] = 0;
                        mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateY) + 1] = 0;
                        mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateAngular)] = 0;
                        mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::CoordinateAngular) + 1] = 0;
                    }
                }
                else if(mbMapping_->tab_bits[static_cast<int>(CoilAddress::Relocation)] == 0 && signalControlCache_.clear_shelf == 1)
                {
                    signalControlCache_.relocation = 0;
                }


            }

            // update robot state
            mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::TargetRes)] = mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TargetReq)];
            mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::TaskIdRes)] = mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TaskIdReq)];
            mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::TaskTypeRes)] = mbMapping_->tab_registers[static_cast<int>(HoldingRegisterAddress::TaskTypeReq)];
            if(getRobotStatusCallback_)
            {
                std::string robot_state_raw = getRobotStatusCallback_();
                Json::Value root;
                Json::CharReaderBuilder builder;
                std::string errs; // Biến lưu thông báo lỗi nếu parse thất bại

                // Tạo bộ đọc reader từ builder
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

                // Thực hiện parse
                bool is_success = reader->parse(
                    robot_state_raw.c_str(),                  // Điểm bắt đầu chuỗi
                    robot_state_raw.c_str() + robot_state_raw.size(),// Điểm kết thúc chuỗi
                    &root,                             // Biến chứa kết quả sau khi parse
                    &errs                              // Biến chứa lỗi
                );

                // Kiểm tra kết quả
                if (!is_success) {
                    std::cout << "Parse JSON robot raw state : " << errs << std::endl;
                    continue;
                }

                if(root.isMember("mission") && root["mission"].isObject())
                {
                    Json::Value mission = root["mission"];

                    if(mission.isMember("mission_code") && mission["mission_code"].isString())
                    {
                        std::string mission_id = mission["mission_code"].asString();
                        if(!mission_id.empty())
                        {
                            uint16_t num;
                            auto [ptr, ec] = std::from_chars(mission_id.data(), mission_id.data() + mission_id.size(), num);
                            if (ec == std::errc()) 
                            {
                                mbMapping_->tab_input_registers[static_cast<uint16_t>(InputRegisterAddress::MissionId)] = num;
                            }
                        }
                    }

                    if(mission.isMember("mission_status") && mission["mission_status"].isInt())
                    {
                        int mission_status = mission["mission_status"].asInt();
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::MissionStatus)] = mission_status;
                    }
                }



                if(root.isMember("lift") && root["lift"].isObject())
                {
                    Json::Value lift = root["lift"];
                    if(lift.isMember("lift_position") && lift["lift_position"].isInt())
                    {
                        int lift_position = lift["lift_position"].asInt();
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::LiftPosition)] = lift_position + 1;
                    }

                    if(lift.isMember("lift_error") && lift["lift_error"].isArray())
                    {
                        const Json::Value& lift_error = lift["lift_error"];
                        if(!lift_error.empty())
                        {
                            const Json::Value& first_error = lift_error[0];
                            mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::LiftError)] = first_error.asInt();
                        }
                        else {
                            mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::LiftError)] = 0;
                        }
                    }
                }

                if(root.isMember("navigator_ip") && root["navigator_ip"].isString())
                {
                    
                    std::string ip_address = root["navigator_ip"].asString();
                    std::stringstream ss(ip_address);
                    // Khai báo 4 biến số nguyên 16-bit (uint16_t hoặc đại loại là uint8_t/int)
                    uint16_t ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
                    char dot; // Biến tạm để hứng các dấu chấm '.'

                    // Đọc luồng dữ liệu theo thứ tự: Số -> Chấm -> Số -> Chấm -> Số -> Chấm -> Số
                    if (ss >> ip1 >> dot >> ip2 >> dot >> ip3 >> dot >> ip4) {
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp)] = ip1;
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp) + 1] = ip2;
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp) + 2] = ip3;
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp) + 3] = ip4;
                    } else {
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp)] = 0;
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp) + 1] = 0;
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp) + 2] = 0;
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigatorIp) + 3] = 0;
                    }
                }

                if(root.isMember("robot_status") && root["robot_status"].isInt())
                {
                    mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::RobotStatus)] = root["robot_status"].asInt();
                }

                if(root.isMember("navigator") && root["navigator"].isObject())
                {
                    
                    const Json::Value& navigator = root["navigator"];

                    // current station
                    if(navigator.isMember("current_station") && navigator["current_station"].isString())
                    {
                        std::string current_station = navigator["current_station"].asString();
                        if(!current_station.empty())
                        {
                            uint16_t num;
                            auto [ptr, ec] = std::from_chars(current_station.substr(2).data(), current_station.substr(2).data() + current_station.substr(2).size(), num);
                            if (ec == std::errc()) 
                            {
                                mbMapping_->tab_input_registers[static_cast<uint16_t>(InputRegisterAddress::CurrentStation)] = num;
                            }
                        }
                    }

                    // target id
                    if(navigator.isMember("target_id") && navigator["target_id"].isString())
                    {
                        std::string target_id = navigator["target_id"].asString();
                        if(!target_id.empty())
                        {
                            uint16_t num;
                            auto [ptr, ec] = std::from_chars(target_id.substr(2).data(), target_id.substr(2).data() + target_id.substr(2).size(), num);
                            if (ec == std::errc()) 
                            {
                                mbMapping_->tab_input_registers[static_cast<uint16_t>(InputRegisterAddress::NextStation)] = num;
                            }
                        }
                    }
                    
                    // current map
                    if(navigator.isMember("current_map") && navigator["current_map"].isString()) 
                    {
                        uint16_t num;
                        std::string map_current = navigator["current_map"].asString();
                        auto [ptr, ec] = std::from_chars(map_current.data(), map_current.data() + map_current.size(), num);
                        if (ec == std::errc()) 
                        {
                            mbMapping_->tab_input_registers[static_cast<uint16_t>(InputRegisterAddress::MapId)] = num;
                        }
                    }

                    // reloc status
                    {
                        mbMapping_->tab_input_registers[static_cast<uint16_t>(InputRegisterAddress::LocationState)] = navigator["reloc_status"].asInt();
                    }

                    if(navigator.isMember("confidence") && navigator["confidence"].isDouble())
                    // confidence
                    {
                        float confidence = navigator["confidence"].asFloat();
                        uint32_t temp;

                        std::memcpy(&temp, &confidence, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::Confidence)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::Confidence) + 1] = static_cast<uint16_t>(temp & 0xFFFF);
                    }
                    
                    // battery
                    {
                        float bat_level = navigator["battery_level"].asFloat();
                        float bat_temp = navigator["battery_temp"].asFloat();
                        float bat_vol = navigator["voltage"].asFloat();
                        float bat_current = navigator["current"].asFloat();
                        uint32_t temp;

                        std::memcpy(&temp, &bat_level, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryLevel)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryLevel) + 1] = static_cast<uint16_t>(temp & 0xFFFF);

                        std::memcpy(&temp, &bat_temp, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryTemp)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryTemp) + 1] = static_cast<uint16_t>(temp & 0xFFFF);

                        std::memcpy(&temp, &bat_vol, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryVol)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryVol) + 1] = static_cast<uint16_t>(temp & 0xFFFF);  
                    
                        std::memcpy(&temp, &bat_current, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryCurrent)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::BatteryCurrent) + 1] = static_cast<uint16_t>(temp & 0xFFFF);

                    }

                    if(navigator.isMember("x") && navigator["x"].isDouble() 
                        && navigator.isMember("y") && navigator["y"].isDouble()
                        && navigator.isMember("angle") && navigator["angle"].isDouble())
                    // coordinate
                    {
                        float x = navigator["x"].asFloat();
                        float y = navigator["y"].asFloat();
                        float angle = navigator["angle"].asFloat();

                        uint32_t temp;

                        std::memcpy(&temp, &x, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::CoordinateX)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::CoordinateX) + 1] = static_cast<uint16_t>(temp & 0xFFFF);

                        std::memcpy(&temp, &y, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::CoordinateY)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::CoordinateY) + 1] = static_cast<uint16_t>(temp & 0xFFFF);

                        std::memcpy(&temp, &angle, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::CoordinateAngular)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::CoordinateAngular) + 1] = static_cast<uint16_t>(temp & 0xFFFF);
                    }

                    if(navigator.isMember("vx") && navigator["vx"].isDouble() 
                        && navigator.isMember("vy") && navigator["vy"].isDouble()
                        && navigator.isMember("w") && navigator["w"].isDouble())
                    // velocity
                    {
                        float vx = navigator["vx"].asFloat();
                        float vy = navigator["vy"].asFloat();
                        float w = navigator["w"].asFloat();

                        uint32_t temp;

                        std::memcpy(&temp, &vx, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::VelocityX)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::VelocityX) + 1] = static_cast<uint16_t>(temp & 0xFFFF);

                        std::memcpy(&temp, &vy, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::VelocityY)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::VelocityY) + 1] = static_cast<uint16_t>(temp & 0xFFFF);

                        std::memcpy(&temp, &w, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::VelocityAngular)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::VelocityAngular) + 1] = static_cast<uint16_t>(temp & 0xFFFF);
                    }

                    // charging
                    if(navigator.isMember("charging") && navigator["charging"].isBool())
                    {
                        bool charging = navigator["charging"].asBool();
                        mbMapping_->tab_input_bits[static_cast<int>(DiscreteInputAddress::Charing)] = charging;
                    }
                    // blocked
                    if(navigator.isMember("blocked") && navigator["blocked"].isBool())
                    {
                        bool blocked = navigator["blocked"].asBool();
                        mbMapping_->tab_input_bits[static_cast<int>(DiscreteInputAddress::Block)] = blocked;
                    }
                    // estop
                    if(navigator.isMember("emergency") && navigator["emergency"].isBool())
                    {
                        bool estop = navigator["emergency"].asBool();
                        mbMapping_->tab_input_bits[static_cast<int>(DiscreteInputAddress::Estop)] = estop;
                    }

                    // odo
                    if(navigator.isMember("odo") && navigator["odo"].isDouble())
                    {
                        float odo = navigator["odo"].asFloat();
                        uint32_t temp;

                        std::memcpy(&temp, &odo, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::TotalDistance)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::TotalDistance) + 1] = static_cast<uint16_t>(temp & 0xFFFF);
                    }

                    // time
                    if(navigator.isMember("time") && navigator["time"].isDouble())
                    {
                        float time = navigator["time"].asFloat();
                        uint32_t temp;

                        std::memcpy(&temp, &time, sizeof(float));
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::Time)] = static_cast<uint16_t>((temp >> 16) & 0xFFFF);
                        mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::Time) + 1] = static_cast<uint16_t>(temp & 0xFFFF);
                    }

                    // error
                    if(navigator.isMember("errors") && navigator["error"].isArray()
                        && navigator.isMember("fatals") && navigator["fatals"].isArray())
                    {
                        const Json::Value& errors = navigator["errors"];
                        const Json::Value& fatals = navigator["fatals"];
                        if(!errors.empty() || !fatals.empty())
                        {
                            mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigationError)] = 1;
                        }
                        else 
                        {
                            mbMapping_->tab_input_registers[static_cast<int>(InputRegisterAddress::NavigationError)] = 0;
                        }
                    }

                    if(navigator.isMember("goods_region") && navigator["goods_region"].isObject())
                    {
                        const Json::Value& goods_region = navigator["goods_region"];
                        if(goods_region.isMember("name") && goods_region["name"].isString())
                        {
                            uint16_t num;
                            std::string shelf_name = goods_region["name"].asString();
                            auto [ptr, ec] = std::from_chars(shelf_name.data(), shelf_name.data() + shelf_name.size(), num);
                            if (ec == std::errc()) 
                            {
                                mbMapping_->tab_input_registers[static_cast<uint16_t>(InputRegisterAddress::ShelfId)] = num;
                            }
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    void ModbusTcpGateway::acceptClientThreadEntry(void)
    {
        while (running_) {
            // Chấp nhận kết nối mới
            int client_socket = modbus_tcp_accept(ctx_, &serverSocket_);
            
            if (client_socket == -1) {
                // Kiểm tra xem có phải timeout không
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Timeout bình thường, tiếp tục
                    continue;
                }
                
                // Lỗi khác
                if (running_.load()) {
                    std::cerr << "[Lỗi] modbus_tcp_accept: " << strerror(errno) << std::endl;
                }
                continue;
            }

            // Kiểm tra giới hạn kết nối
            int current_clients = activeClients_.load();
            if (current_clients >= 10) {
                std::cerr << "[Từ chối] Đã đạt giới hạn 10 client. Ngắt kết nối từ " 
                        << client_socket << std::endl;
                close(client_socket);
                continue;
            }

            // Chấp nhận kết nối
            clientCount_++;
            // std::cout << "[Hệ thống] Kết nối mới #" << clientCount_ << " (Tổng: " << (current_clients + 1) << "/10)" << std::endl;

            try {
                std::lock_guard<std::mutex> lock(threadsMutex_);
                // Đẩy trực tiếp thread mới vào vector
                clientThreads_.emplace_back([this, client_socket]() {
                    this->clientHandler(client_socket);
                });
            } catch (const std::exception& e) {
                std::cerr << "[Lỗi] Không thể tạo thread: " << e.what() << std::endl;
                close(client_socket);
            }
        }
    }

    // sendStatus
    gateway::domain::entities::NetworkResult ModbusTcpGateway::sendStatus(const std::string& payload)
    {
        return domain::entities::NetworkResult{domain::entities::NetworkStatus::Success};
    }
            
    // sendRequest
    gateway::domain::entities::NetworkResult ModbusTcpGateway::sendRequest(const std::string& payload)
    {
        return domain::entities::NetworkResult{domain::entities::NetworkStatus::Success};
    }

    // sendReponse
    gateway::domain::entities::NetworkResult ModbusTcpGateway::sendResponse(const std::string& payload)
    {
        return domain::entities::NetworkResult{domain::entities::NetworkStatus::Success};
    }

    void ModbusTcpGateway::setGatewayEventCallback(GatewayEventCallback cb)
    {
        eventCallback_ = cb;
    }

    void ModbusTcpGateway::setGatewayGetRobotCallback(GatewayGetRobotStateCallback cb)
    {
        getRobotStatusCallback_ = cb;
    }
}