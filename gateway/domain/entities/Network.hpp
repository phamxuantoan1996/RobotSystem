#pragma once
#include <string>
namespace gateway::domain::entities {
    enum class NetworkStatus {
        Success,       // Gửi thành công, Fleet nhận tốt
        ConnectionError, // Mất mạng, không kết nối được Server/Broker
        Timeout,       // Server không phản hồi kịp thời
        ServerError    // Kết nối được nhưng Server trả về lỗi (Ví dụ: HTTP 500, 400)
    };

    struct NetworkResult {
        NetworkStatus status;
        int code = 0;              // HTTP Status Code (200, 404, 500) hoặc mã lỗi MQTT
        std::string error_message; // Chi tiết lỗi để ghi Log khi debug
    };
}