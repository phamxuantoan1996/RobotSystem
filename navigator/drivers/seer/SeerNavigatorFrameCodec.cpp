#include "SeerNavigatorFrameCodec.hpp"
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

namespace navigator::drivers::seer {
    
    bool SeerNavigatorFrameCodec::isValidHeader(const uint8_t header[HEADER_SIZE])
    {
        return  header[0] == SEER_MAGIC && (header[1] == SEER_VERSION_34 || header[1] == SEER_VERSION_35);
    }

    uint32_t SeerNavigatorFrameCodec::payloadLength(const uint8_t header[HEADER_SIZE])
    {
        return (static_cast<uint32_t>(header[4]) << 24) 
                | (static_cast<uint32_t>(header[5]) << 16) 
                | (static_cast<uint32_t>(header[6]) << 8) 
                | static_cast<uint32_t>(header[7]);
    }

    std::vector<uint8_t> SeerNavigatorFrameCodec::encode(const SeerNavigatorFrame& frame)
    {
        const uint32_t payLen = static_cast<uint32_t>(frame.payload.size());
        std::vector<uint8_t> buf(HEADER_SIZE + payLen, 0);

        // sync
        buf[0] = SEER_MAGIC;

        // version
        buf[1] = frame.version;

        // serial - big-endian uint16
        buf[2] = static_cast<uint8_t>((frame.serial >> 8) & 0xFF);
        buf[3] = static_cast<uint8_t>(frame.serial & 0xFF);

        // length of payload - big-endian
        buf[4] = static_cast<uint8_t>((payLen >> 24) & 0xFF);
        buf[5] = static_cast<uint8_t>((payLen >> 16) & 0xFF);
        buf[6] = static_cast<uint8_t>((payLen >> 8) & 0xFF);
        buf[7] = static_cast<uint8_t>(payLen & 0xFF);

        // m_type (API number) - big-endian
        buf[8] = static_cast<uint8_t>((frame.msgType >> 8) & 0xFF);
        buf[9] = static_cast<uint8_t>(frame.msgType & 0xFF);

        if(payLen > 0)
        {
            std::memcpy(buf.data() + HEADER_SIZE, frame.payload.data(), payLen);
        }

        return  buf;
    }

    std::optional<SeerNavigatorFrame> SeerNavigatorFrameCodec::decode(const std::vector<uint8_t>& bytes)
    {
        if(bytes.size() < HEADER_SIZE)
        {
            return std::nullopt;
        }
        if(!isValidHeader(bytes.data()))
        {
            return std::nullopt;
        }

        const uint32_t payLen = payloadLength(bytes.data());
        if(bytes.size() < HEADER_SIZE + payLen)
        {
            return std::nullopt;
        }

        SeerNavigatorFrame frame;
        frame.version = bytes[1];
        frame.serial = static_cast<uint16_t>((bytes[2] << 8) | bytes[3]);
        frame.msgType = static_cast<uint16_t>((bytes[8] << 8) | bytes[9]);
        if(payLen > 0)
        {
            frame.payload.assign(reinterpret_cast<const char*>(bytes.data() + HEADER_SIZE),payLen);
        }

        return frame;
    } 
}