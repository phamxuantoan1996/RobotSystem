#include "RestGateway.hpp"
#include <drogon/drogon.h>
#include <iostream>
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
        auto client = drogon::HttpClient::newHttpClient(fleetUrl_);
        
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/api/v1/fleet/webhook/mission/status");
        req->setBody(payload);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        // 🚀 Thừa hành thiết lập Timeout 2.0 giây
        auto [req_result, response] = client->sendRequest(req, 2.0); 

        if (req_result == drogon::ReqResult::Timeout) {
            result.status = domain::entities::NetworkStatus::Timeout; // Trả về Enum lỗi Timeout của bạn
            result.code = 408; // Mã HTTP Timeout tiêu chuẩn
            result.error_message = "Ket noi den Fleet Server bi qua han (Timeout 2s).";
            return result;
        }

        // Các logic check kết quả Ok hoặc ServerError giữ nguyên...
        if (req_result == drogon::ReqResult::Ok && response) {
            result.code = response->getStatusCode();
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
    
    // sendRequest
    gateway::domain::entities::NetworkResult RestGateway::sendRequest(const std::string& payload)
    {
        return domain::entities::NetworkResult{domain::entities::NetworkStatus::Success};
    }

    // sendReponse
    gateway::domain::entities::NetworkResult RestGateway::sendResponse(const std::string& payload)
    {
        return domain::entities::NetworkResult{domain::entities::NetworkStatus::Success};
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

    // thread server
    void RestGateway::drogonServerThread()
    {
        drogon::app().addListener("0.0.0.0", port_);
        drogon::app().registerHandler(
            "/dispatch_mission",
            [](const drogon::HttpRequestPtr& req, 
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) 
            {
                // Lấy chuỗi String thô từ HTTP Body (Fleet gửi xuống)
                std::string raw_payload = std::string(req->getBody());
                std::cout << "raw payload : \n" << raw_payload << std::endl;

                // // Đóng gói vào cấu trúc Event thô của bạn
                // robot::infrastructure::events::IncomingCommandEvent event{ raw_payload };

                // // Bắn lên EventBus để RobotController xử lý bất đồng bộ ở tầng dưới
                // common::bus::EventBus::getInstance().publish(std::move(event));

                // 🚀 ĐÚNG Ý TƯỞNG: Trả về mã HTTP 202 (Accepted) ngay lập tức cho Fleet
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k202Accepted);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(R"({"status":"Accepted","message":"Mission pushed to EventBus"})");
                callback(resp);
            },
            {drogon::Post} // Chỉ chấp nhận phương thức POST
        );
        std::cout << "[Drogon Gateway] HTTP Server dang chay tren port " << port_ << "...\n";
        drogon::app().run(); 
    }
}