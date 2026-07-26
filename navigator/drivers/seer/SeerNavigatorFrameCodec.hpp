#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <stdint.h>
#include <string>
#include <optional>

namespace navigator::drivers::seer {

    static constexpr uint8_t SEER_MAGIC = 0x5A;
    static constexpr uint8_t SEER_VERSION_34 = 0x01;
    static constexpr uint8_t SEER_VERSION_35 = 0x02;
    static constexpr size_t HEADER_SIZE = 16;

    struct SeerNavigatorFrame {
        uint8_t version = SEER_VERSION_34;
        uint16_t serial = 0;
        uint16_t msgType = 0;
        std::string payload;
    };    

    class SeerNavigatorFrameCodec {
        public:
            // mã hóa command request thành mảng byte
            static std::vector<uint8_t> encode(const SeerNavigatorFrame& frame);

            // giải mã mảng bytes phản hồi từ seer
            static std::optional<SeerNavigatorFrame> decode(const std::vector<uint8_t>& byte);

            // truyền vào mảng chứa header để xác định kích thước payload
            static uint32_t payloadLength(const uint8_t header[HEADER_SIZE]);

            // truyền vào mảng chứa header để xác định response có hợp lệ không
            static bool isValidHeader(const uint8_t header[HEADER_SIZE]);
    };
}