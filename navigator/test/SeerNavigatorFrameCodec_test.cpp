#include <gtest/gtest.h>
#include <cstring>
#include "SeerNavigatorFrameCodec.hpp"

using namespace navigator::drivers::seer;

// ============================================
// TEST FIXTURE - Khởi tạo dữ liệu test chung
// ============================================
class SeerNavigatorFrameCodecTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Không cần khởi tạo gì vì class toàn static
    }
    
    // Helper: Tạo frame mẫu
    SeerNavigatorFrame createSampleFrame(uint16_t serial = 0x1234, 
                                         uint16_t msgType = 0x5678,
                                         const std::string& payload = "Hello") {
        SeerNavigatorFrame frame;
        frame.version = SEER_VERSION_34;
        frame.serial = serial;
        frame.msgType = msgType;
        frame.payload = payload;
        return frame;
    }
    
    // Helper: Tạo header mẫu
    void createSampleHeader(uint8_t header[HEADER_SIZE], 
                           uint8_t version = SEER_VERSION_34,
                           uint32_t payloadLen = 10) {
        memset(header, 0, HEADER_SIZE);
        header[0] = SEER_MAGIC;
        header[1] = version;
        // payload length (big-endian)
        header[4] = (payloadLen >> 24) & 0xFF;
        header[5] = (payloadLen >> 16) & 0xFF;
        header[6] = (payloadLen >> 8) & 0xFF;
        header[7] = payloadLen & 0xFF;
    }
};

// ============================================
// 1. TEST isValidHeader
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, IsValidHeader_ValidHeader_ReturnsTrue) {
    // Arrange
    uint8_t header[HEADER_SIZE];
    createSampleHeader(header, SEER_VERSION_34);
    
    // Act
    bool result = SeerNavigatorFrameCodec::isValidHeader(header);
    
    // Assert
    EXPECT_TRUE(result);
}

TEST_F(SeerNavigatorFrameCodecTest, IsValidHeader_ValidVersion35_ReturnsTrue) {
    // Arrange
    uint8_t header[HEADER_SIZE];
    createSampleHeader(header, SEER_VERSION_35);
    
    // Act
    bool result = SeerNavigatorFrameCodec::isValidHeader(header);
    
    // Assert
    EXPECT_TRUE(result);
}

TEST_F(SeerNavigatorFrameCodecTest, IsValidHeader_WrongMagic_ReturnsFalse) {
    // Arrange
    uint8_t header[HEADER_SIZE] = {0};
    header[0] = 0xFF;  // Wrong magic
    
    // Act
    bool result = SeerNavigatorFrameCodec::isValidHeader(header);
    
    // Assert
    EXPECT_FALSE(result);
}

TEST_F(SeerNavigatorFrameCodecTest, IsValidHeader_WrongVersion_ReturnsFalse) {
    // Arrange
    uint8_t header[HEADER_SIZE] = {0};
    header[0] = SEER_MAGIC;
    header[1] = 0xFF;  // Wrong version
    
    // Act
    bool result = SeerNavigatorFrameCodec::isValidHeader(header);
    
    // Assert
    EXPECT_FALSE(result);
}

TEST_F(SeerNavigatorFrameCodecTest, IsValidHeader_ZeroHeader_ReturnsFalse) {
    // Arrange
    uint8_t header[HEADER_SIZE] = {0};
    
    // Act
    bool result = SeerNavigatorFrameCodec::isValidHeader(header);
    
    // Assert
    EXPECT_FALSE(result);
}

// ============================================
// 2. TEST payloadLength
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, PayloadLength_ValidHeader_ReturnsCorrectLength) {
    // Arrange
    uint8_t header[HEADER_SIZE];
    uint32_t expectedLen = 0x12345678;
    createSampleHeader(header, SEER_VERSION_34, expectedLen);
    
    // Act
    uint32_t result = SeerNavigatorFrameCodec::payloadLength(header);
    
    // Assert
    EXPECT_EQ(result, expectedLen);
}

TEST_F(SeerNavigatorFrameCodecTest, PayloadLength_ZeroLength_ReturnsZero) {
    // Arrange
    uint8_t header[HEADER_SIZE];
    createSampleHeader(header, SEER_VERSION_34, 0);
    
    // Act
    uint32_t result = SeerNavigatorFrameCodec::payloadLength(header);
    
    // Assert
    EXPECT_EQ(result, 0);
}

TEST_F(SeerNavigatorFrameCodecTest, PayloadLength_MaxLength_ReturnsCorrect) {
    // Arrange
    uint8_t header[HEADER_SIZE];
    uint32_t maxLen = 0xFFFFFFFF;
    createSampleHeader(header, SEER_VERSION_34, maxLen);
    
    // Act
    uint32_t result = SeerNavigatorFrameCodec::payloadLength(header);
    
    // Assert
    EXPECT_EQ(result, maxLen);
}

// ============================================
// 3. TEST encode
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, Encode_ValidFrame_ReturnsCorrectBytes) {
    // Arrange
    SeerNavigatorFrame frame = createSampleFrame(0x1234, 0x5678, "Hello");
    
    // Act
    std::vector<uint8_t> result = SeerNavigatorFrameCodec::encode(frame);
    
    // Assert
    // Header size
    EXPECT_EQ(result.size(), HEADER_SIZE + 5); // 16 + 5 (payload "Hello")
    
    // Magic
    EXPECT_EQ(result[0], SEER_MAGIC);
    
    // Version
    EXPECT_EQ(result[1], SEER_VERSION_34);
    
    // Serial (big-endian)
    EXPECT_EQ(result[2], 0x12);
    EXPECT_EQ(result[3], 0x34);
    
    // Payload length (big-endian)
    EXPECT_EQ(result[4], 0x00);
    EXPECT_EQ(result[5], 0x00);
    EXPECT_EQ(result[6], 0x00);
    EXPECT_EQ(result[7], 0x05);  // Length = 5
    
    // MsgType (big-endian)
    EXPECT_EQ(result[8], 0x56);
    EXPECT_EQ(result[9], 0x78);
    
    // Payload
    std::string payload(reinterpret_cast<const char*>(result.data() + HEADER_SIZE), 5);
    EXPECT_EQ(payload, "Hello");
}

TEST_F(SeerNavigatorFrameCodecTest, Encode_EmptyPayload_ReturnsCorrectSize) {
    // Arrange
    SeerNavigatorFrame frame = createSampleFrame(0x0001, 0x0002, "");
    
    // Act
    std::vector<uint8_t> result = SeerNavigatorFrameCodec::encode(frame);
    
    // Assert
    EXPECT_EQ(result.size(), HEADER_SIZE);  // Only header, no payload
    EXPECT_EQ(result[4], 0x00);
    EXPECT_EQ(result[5], 0x00);
    EXPECT_EQ(result[6], 0x00);
    EXPECT_EQ(result[7], 0x00);  // Length = 0
}

TEST_F(SeerNavigatorFrameCodecTest, Encode_LargePayload_HandlesCorrectly) {
    // Arrange
    std::string largePayload(1000, 'A');  // 1000 bytes
    SeerNavigatorFrame frame = createSampleFrame(0, 0, largePayload);
    
    // Act
    std::vector<uint8_t> result = SeerNavigatorFrameCodec::encode(frame);
    
    // Assert
    EXPECT_EQ(result.size(), HEADER_SIZE + 1000);
    
    // Verify payload length encoding
    uint32_t len = (result[4] << 24) | (result[5] << 16) | (result[6] << 8) | result[7];
    EXPECT_EQ(len, 1000);
    
    // Verify payload content
    std::string payload(reinterpret_cast<const char*>(result.data() + HEADER_SIZE), 1000);
    EXPECT_EQ(payload, largePayload);
}

TEST_F(SeerNavigatorFrameCodecTest, Encode_MultipleFrames_Consistent) {
    // Arrange
    std::vector<SeerNavigatorFrame> frames = {
        createSampleFrame(1, 100, "First"),
        createSampleFrame(2, 200, "Second"),
        createSampleFrame(3, 300, "Third")
    };
    
    // Act & Assert
    for (const auto& frame : frames) {
        auto encoded = SeerNavigatorFrameCodec::encode(frame);
        auto decoded = SeerNavigatorFrameCodec::decode(encoded);
        
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(decoded->serial, frame.serial);
        EXPECT_EQ(decoded->msgType, frame.msgType);
        EXPECT_EQ(decoded->payload, frame.payload);
    }
}

// ============================================
// 4. TEST decode
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, Decode_ValidBytes_ReturnsFrame) {
    // Arrange
    SeerNavigatorFrame original = createSampleFrame(0x1234, 0x5678, "Hello");
    std::vector<uint8_t> bytes = SeerNavigatorFrameCodec::encode(original);
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, original.version);
    EXPECT_EQ(result->serial, original.serial);
    EXPECT_EQ(result->msgType, original.msgType);
    EXPECT_EQ(result->payload, original.payload);
}

TEST_F(SeerNavigatorFrameCodecTest, Decode_EmptyPayload_ReturnsFrame) {
    // Arrange
    SeerNavigatorFrame original = createSampleFrame(0x1234, 0x5678, "");
    std::vector<uint8_t> bytes = SeerNavigatorFrameCodec::encode(original);
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->payload, "");
}

TEST_F(SeerNavigatorFrameCodecTest, Decode_BytesTooShort_ReturnsNullopt) {
    // Arrange
    std::vector<uint8_t> bytes(HEADER_SIZE - 1, 0x00);  // Thiếu 1 byte
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    EXPECT_FALSE(result.has_value());
}

TEST_F(SeerNavigatorFrameCodecTest, Decode_InvalidHeader_ReturnsNullopt) {
    // Arrange
    std::vector<uint8_t> bytes(HEADER_SIZE, 0xFF);  // Header sai
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    EXPECT_FALSE(result.has_value());
}

TEST_F(SeerNavigatorFrameCodecTest, Decode_MissingPayload_ReturnsNullopt) {
    // Arrange
    SeerNavigatorFrame frame = createSampleFrame(0, 0, "Hello");
    auto bytes = SeerNavigatorFrameCodec::encode(frame);
    bytes.resize(HEADER_SIZE + 3);  // Thiếu payload
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    EXPECT_FALSE(result.has_value());
}

TEST_F(SeerNavigatorFrameCodecTest, Decode_ExtraBytes_StillDecodesCorrectly) {
    // Arrange
    SeerNavigatorFrame original = createSampleFrame(0x1234, 0x5678, "Hello");
    auto bytes = SeerNavigatorFrameCodec::encode(original);
    bytes.push_back(0xFF);  // Extra byte
    bytes.push_back(0xEE);  // Extra byte
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->serial, original.serial);
    EXPECT_EQ(result->payload, original.payload);
    // Extra bytes ignored (decode chỉ đọc đúng payload length)
}

TEST_F(SeerNavigatorFrameCodecTest, Decode_Version35_HandlesCorrectly) {
    // Arrange
    SeerNavigatorFrame original = createSampleFrame(0x1234, 0x5678, "Hello");
    original.version = SEER_VERSION_35;  // Phiên bản mới
    auto bytes = SeerNavigatorFrameCodec::encode(original);
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->version, SEER_VERSION_35);
    EXPECT_EQ(result->serial, original.serial);
    EXPECT_EQ(result->payload, original.payload);
}

// ============================================
// 5. TEST ENCODE + DECODE INTEGRATION
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, EncodeDecode_RoundTrip_MaintainsData) {
    // Arrange
    std::vector<SeerNavigatorFrame> testCases = {
        createSampleFrame(0x0001, 0x0001, "A"),
        createSampleFrame(0xFFFF, 0xFFFF, std::string(256, 'X')),
        createSampleFrame(0x1234, 0x5678, "Special chars: !@#$%^&*()"),
        createSampleFrame(0x0000, 0x0000, ""),
        createSampleFrame(0x7FFF, 0x8000, "Mixed case Data")
    };
    
    // Act & Assert
    for (const auto& original : testCases) {
        auto encoded = SeerNavigatorFrameCodec::encode(original);
        auto decoded = SeerNavigatorFrameCodec::decode(encoded);
        
        ASSERT_TRUE(decoded.has_value()) << "Decode failed for serial: " << original.serial;
        EXPECT_EQ(decoded->version, original.version);
        EXPECT_EQ(decoded->serial, original.serial);
        EXPECT_EQ(decoded->msgType, original.msgType);
        EXPECT_EQ(decoded->payload, original.payload);
    }
}

// ============================================
// 6. TEST Edge Cases & Boundary
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, PayloadLength_BigEndianOrder_IsCorrect) {
    // Arrange
    uint8_t header[HEADER_SIZE] = {0};
    header[0] = SEER_MAGIC;
    header[4] = 0x12;
    header[5] = 0x34;
    header[6] = 0x56;
    header[7] = 0x78;
    
    // Act
    uint32_t result = SeerNavigatorFrameCodec::payloadLength(header);
    
    // Assert
    EXPECT_EQ(result, 0x12345678);
}

TEST_F(SeerNavigatorFrameCodecTest, Encode_SerialBigEndian_IsCorrect) {
    // Arrange
    SeerNavigatorFrame frame = createSampleFrame(0xABCD, 0, "");
    
    // Act
    auto result = SeerNavigatorFrameCodec::encode(frame);
    
    // Assert
    EXPECT_EQ(result[2], 0xAB);
    EXPECT_EQ(result[3], 0xCD);
}

TEST_F(SeerNavigatorFrameCodecTest, Decode_SerialBigEndian_IsCorrect) {
    // Arrange
    std::vector<uint8_t> bytes(HEADER_SIZE, 0);
    bytes[0] = SEER_MAGIC;
    bytes[1] = SEER_VERSION_34;
    bytes[2] = 0xAB;
    bytes[3] = 0xCD;
    bytes[4] = 0x00;
    bytes[5] = 0x00;
    bytes[6] = 0x00;
    bytes[7] = 0x00;  // Payload length = 0
    bytes[8] = 0x00;
    bytes[9] = 0x00;
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(bytes);
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->serial, 0xABCD);
}

// ============================================
// 7. TEST Performance/Benchmark (optional)
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, EncodeDecode_LargeData_HandlesEfficiently) {
    // Arrange
    std::string largePayload(10000, 'X');  // 10KB
    SeerNavigatorFrame frame = createSampleFrame(1, 1, largePayload);
    
    // Act
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    // Assert
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->payload, largePayload);
    EXPECT_EQ(encoded.size(), HEADER_SIZE + 10000);
}

// ============================================
// 8. TEST Error Cases
// ============================================

TEST_F(SeerNavigatorFrameCodecTest, Decode_NullData_ReturnsNullopt) {
    // Arrange
    std::vector<uint8_t> emptyBytes;
    
    // Act
    auto result = SeerNavigatorFrameCodec::decode(emptyBytes);
    
    // Assert
    EXPECT_FALSE(result.has_value());
}

TEST_F(SeerNavigatorFrameCodecTest, IsValidHeader_NullHeader_UndefinedBehavior) {
    // Lưu ý: Không test nullptr vì đây là undefined behavior
    // Chỉ test với header đã được khởi tạo
    uint8_t header[HEADER_SIZE] = {0};
    EXPECT_FALSE(SeerNavigatorFrameCodec::isValidHeader(header));
}

TEST_F(SeerNavigatorFrameCodecTest, PayloadLength_NullHeader_UndefinedBehavior) {
    // Lưu ý: Không test nullptr vì đây là undefined behavior
    uint8_t header[HEADER_SIZE] = {0};
    // Không assert gì, chỉ đảm bảo không crash
    (void)SeerNavigatorFrameCodec::payloadLength(header);
}