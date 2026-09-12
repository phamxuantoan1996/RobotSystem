#include "RestGateway.hpp"
#include "GatewayEvent.hpp"
#include "SignalType.hpp"
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <iostream>
#include <json/value.h>
#include <string>
#include <utility>
namespace gateway::drivers::rest {
    RestGateway::RestGateway(const std::string& fleet_url, uint16_t listen_port) : fleetUrl_(fleet_url),port_(listen_port)
    {
        
    }
    RestGateway::~RestGateway()
    {
        stop();
    }

    // sendStatus
    gateway::domain::entities::NetworkResult RestGateway::sendStatus(const std::string& payload)
    {
        domain::entities::NetworkResult result;

        // Sử dụng biến hằng số post_status_endpoint của bạn
        auto client = drogon::HttpClient::newHttpClient(std::string(fleetUrl_));
        
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath(std::string(post_robot_status_endpoint)); 
        req->setBody(payload);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        // SỬA TẠI ĐÂY: Gọi chính hàm sendRequest(req, timeout) để kích hoạt cơ chế đồng bộ
        // Cú pháp Structured Binding [req_result, response] hoàn toàn hợp lệ ở đây!
        auto [req_result, response] = client->sendRequest(req, 2.0); 

        // Logic kiểm tra lỗi Timeout 
        if (req_result == drogon::ReqResult::Timeout) {
            result.status = domain::entities::NetworkStatus::Timeout; 
            result.code = 408; 
            result.error_message = "Ket noi den Fleet Server bi qua han (Timeout 2s).";
            return result;
        }

        // Logic kiểm tra phản hồi từ Server
        if (req_result == drogon::ReqResult::Ok && response) {
            result.code = static_cast<int>(response->getStatusCode());
            if (result.code >= 200 && result.code < 300) {
                result.status = domain::entities::NetworkStatus::Success;

                std::string collision = std::string(response->getBody());
                Json::Value root;
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                std::string errors;
                bool is_parsed_success = reader->parse(
                    collision.c_str(), 
                    collision.c_str() + collision.length(), 
                    &root, 
                    &errors
                );
                
                if (!is_parsed_success) {
                    std::cerr << "Lỗi Cú Pháp JSON: " << errors << std::endl;
                }
                else {
                    if(root.isMember("collision") && root["collision"].isInt())
                    {
                        gateway::domain::entities::CollisionSignalType temp;
                        switch(static_cast<gateway::domain::entities::CollisionSignalType>(root["collision"].asInt()))
                        {
                            case gateway::domain::entities::CollisionSignalType::Exception:
                            {
                                temp = gateway::domain::entities::CollisionSignalType::Exception;
                                break;
                            }
                            case gateway::domain::entities::CollisionSignalType::Run:
                            {
                                temp = gateway::domain::entities::CollisionSignalType::Run;
                                break;
                            }
                            case gateway::domain::entities::CollisionSignalType::Stop:
                            {
                                temp = gateway::domain::entities::CollisionSignalType::Stop;
                                break;
                            }
                            default:
                            {
                                temp = gateway::domain::entities::CollisionSignalType::Unknown;
                            }
                        }
                        if(temp != gateway::domain::entities::CollisionSignalType::Unknown && temp != collision_type)
                        {
                            collision_type = temp;
                            if(eventCallback_)
                            {
                                eventCallback_(gateway::domain::events::SignalCollisionEvent{.collision_type = collision_type});
                            }
                        }
                    }
                }

            } else {
                result.status = domain::entities::NetworkStatus::ServerError;
                result.error_message = "Server tra ve loi HTTP: " + std::to_string(result.code);
            }
        } 
        else 
        {
            result.status = domain::entities::NetworkStatus::ConnectionError;
            result.code = static_cast<int>(req_result);
            result.error_message = "Loi ket noi mang.";
        }

        return result;
    }


    // sendRequest
    gateway::domain::entities::NetworkResult RestGateway::sendRequest(const std::string& payload)
    {
        return domain::entities::NetworkResult{domain::entities::NetworkStatus::Success};
    }

    // sendReponse
    gateway::domain::entities::NetworkResult RestGateway::sendResponse(const std::string& payload)
    {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errors;
        bool is_parsed_success = reader->parse(
            payload.c_str(), 
            payload.c_str() + payload.length(), 
            &root, 
            &errors
        );
        if (!is_parsed_success) {
            return domain::entities::NetworkResult{domain::entities::NetworkStatus::PayloadError};
        }

        std::string endpoint = "";
        if(root.isMember("mission_status") && root["mission_status"].isInt())
        {
            endpoint = std::string(post_mission_status_endpoint);
        }
        else if(root.isMember("robot_error") && root["robot_error"].isObject())
        {
            endpoint = std::string(post_robot_error_endpoint);
        }
        
        if(endpoint.empty())
        {
            return domain::entities::NetworkResult{domain::entities::NetworkStatus::PayloadError};
        }
        
        domain::entities::NetworkResult result;
        // Sử dụng biến hằng số post_status_endpoint của bạn
        auto client = drogon::HttpClient::newHttpClient(std::string(fleetUrl_));
        
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath(std::string(endpoint)); 
        req->setBody(payload);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        // 🚀 SỬA TẠI ĐÂY: Gọi chính hàm sendRequest(req, timeout) để kích hoạt cơ chế đồng bộ
        // Cú pháp Structured Binding [req_result, response] hoàn toàn hợp lệ ở đây!
        auto [req_result, response] = client->sendRequest(req, 2.0); 

        // Logic kiểm tra lỗi Timeout 
        if (req_result == drogon::ReqResult::Timeout) {
            result.status = domain::entities::NetworkStatus::Timeout; 
            result.code = 408; 
            result.error_message = "Ket noi den Fleet Server bi qua han (Timeout 2s).";
            return result;
        }

        // Logic kiểm tra phản hồi từ Server
        if (req_result == drogon::ReqResult::Ok && response) {
            result.code = static_cast<int>(response->getStatusCode());
            if (result.code >= 200 && result.code < 300) {
                result.status = domain::entities::NetworkStatus::Success;
            } else {
                result.status = domain::entities::NetworkStatus::ServerError;
                result.error_message = "Server tra ve loi HTTP: " + std::to_string(result.code);
            }
        } 
        else 
        {
            result.status = domain::entities::NetworkStatus::ConnectionError;
            result.code = static_cast<int>(req_result);
            result.error_message = "Loi ket noi mang.";
        }

        return result;
    }

    // start
    void RestGateway::start()
    {
        if (running_) 
            return;
        running_ = true;

        // Chạy Drogon Server trên một luồng riêng để không block luồng chính
        drogonThread_ = std::thread(&RestGateway::drogonServerThread, this);
    }

    // stop
    void RestGateway::stop()
    {
        if (!running_) 
            return;
        running_ = false;

        // Ra lệnh tắt vòng lặp sự kiện của Drogon một cách an toàn luồng
        drogon::app().quit();

        if (drogonThread_.joinable()) {
            drogonThread_.join();
        }
        std::cout << "[Drogon Gateway] Da dung HTTP Server.\n";
    }
    

    void RestGateway::setGatewayGetRobotCallback(GatewayGetRobotStateCallback cb)
    {
        getRobotStatusCallback_ = cb;
    }

    // thread server
    void RestGateway::drogonServerThread()
    {
        drogon::app().addListener("0.0.0.0", port_);
        drogon::app().registerHandler(
            "/dispatch_mission",
            [this](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                // Lấy chuỗi String thô từ HTTP Body (Fleet gửi xuống)
                std::string raw_payload = std::string(req->getBody());
                std::cout << "raw payload : \n" << raw_payload << std::endl;
                if(eventCallback_)
                {
                    eventCallback_(gateway::domain::events::MissionDispatchEvent{.mission = raw_payload});
                }
                // 🚀 ĐÚNG Ý TƯỞNG: Trả về mã HTTP 202 (Accepted) ngay lập tức cho Fleet
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k202Accepted);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(R"({"status":"Accepted","message":"Mission pushed to EventBus"})");
                callback(resp);
            },
            {drogon::Post} // Chỉ chấp nhận phương thức POST
        );
        drogon::app().registerHandler("/cancel", [this](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                std::cout << "[Drogon Gateway] Received Cancel Request\n";
                // TODO: Thêm logic EventBus hủy nhiệm vụ tại đây
                if(eventCallback_)
                    eventCallback_(gateway::domain::events::SignalCancelEvent{});

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k202Accepted);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(R"({"status":"Accepted","message":"Cancel command received"})");
                callback(resp); // Bắt buộc phải gọi callback
            },{drogon::Post}
        );
        drogon::app().registerHandler("/pause", [this](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                std::cout << "[Drogon Gateway] Received Pause Request\n";
                // TODO: Thêm logic EventBus hủy nhiệm vụ tại đây
                if(eventCallback_)
                    eventCallback_(gateway::domain::events::SignalPauseEvent{});
                
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k202Accepted);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(R"({"status":"Accepted","message":"Pause command received"})");
                callback(resp); // Bắt buộc phải gọi callback
            },{drogon::Post}
        );
        drogon::app().registerHandler("/resume", [this](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                std::cout << "[Drogon Gateway] Received Resume Request\n";
                // TODO: Thêm logic EventBus hủy nhiệm vụ tại đây
                if(eventCallback_)
                    eventCallback_(gateway::domain::events::SignalResumeEvent{});
                
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k202Accepted);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(R"({"status":"Accepted","message":"Resume command received"})");
                callback(resp); // Bắt buộc phải gọi callback
            },{drogon::Post}
        );
        drogon::app().registerHandler("/operation_mode", [this](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                std::cout << "[Drogon Gateway] Received Switch Mode Request\n";
                // TODO: Thêm logic EventBus hủy nhiệm vụ tại đây
                std::string raw_payload = std::string(req->getBody());
                if(eventCallback_)
                    eventCallback_(gateway::domain::events::SignalSwitchModeEvent{.mode = raw_payload});
                
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k202Accepted);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(R"({"status":"Accepted","message":"Switch Mode command received"})");
                callback(resp); // Bắt buộc phải gọi callback
            },{drogon::Post}
        );
        drogon::app().registerHandler("/clear_error", [this](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                std::cout << "[Drogon Gateway] Received Clear Error Request\n";
                // TODO: Thêm logic EventBus hủy nhiệm vụ tại đây
                if(eventCallback_)
                    eventCallback_(gateway::domain::events::SignalClearErrorEvent{});
                
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k202Accepted);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(R"({"status":"Accepted","message":"Clear error command received"})");
                callback(resp); // Bắt buộc phải gọi callback
            },{drogon::Post}
        );
        drogon::app().registerHandler("/status", [this](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                // TODO: Thêm logic EventBus hủy nhiệm vụ tại đây
                std::string status = "";
                if(getRobotStatusCallback_)
                {
                    status = getRobotStatusCallback_();
                }
                
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK); // Hoặc giữ k202Accepted tùy logic của bạn
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(status); // Gửi thẳng chuỗi JSON này về client
                callback(resp); 
            },{drogon::Get}
        );



        std::cout << "[Drogon Gateway] HTTP Server dang chay tren port " << port_ << "  ...\n";
        drogon::app().run(); 
    }

    void RestGateway::setGatewayEventCallback(GatewayEventCallback cb)
    {
        eventCallback_ = std::move(cb);
    }
}