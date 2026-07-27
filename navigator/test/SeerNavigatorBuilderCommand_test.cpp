#include <cstdint>
#include <gtest/gtest.h>
#include <jsoncpp/json/json.h>
#include <cmath>
#include <limits>
#include <vector>
#include <string>
#include <algorithm>
#include "SeerNavigatorCommandBuilder.hpp"
#include "SeerNavigatorFrameCodec.hpp"
#include "NavigatorState.hpp"

using namespace navigator::drivers::seer;
using namespace navigator::domain::entities;

// ============================================
// TEST FIXTURE CLASS
// ============================================
class SeerNavigatorCommandBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        builder_ = new SeerNavigatorCommandBuilder();
    }
    
    void TearDown() override {
        delete builder_;
        builder_ = nullptr;
    }
    
    // ============================================
    // HELPER METHODS
    // ============================================
    
    // Kiểm tra frame cơ bản
    void validateBasicFrame(const SeerNavigatorFrame& frame, 
                           uint16_t expectedMsgType) {
        EXPECT_EQ(frame.version, SEER_VERSION_34);
        EXPECT_GT(frame.serial, 0);
        EXPECT_LE(frame.serial, 65535);
        EXPECT_EQ(frame.msgType, expectedMsgType);
    }
    
    // Parse JSON từ payload
    Json::Value parsePayload(const std::string& payload) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream ss(payload);
        
        if (!Json::parseFromStream(builder, ss, &root, &errs)) {
            throw std::runtime_error("Failed to parse JSON: " + errs);
        }
        return root;
    }
    
    // Kiểm tra JSON có field không
    bool hasField(const Json::Value& root, const std::string& field) {
        return root.isMember(field);
    }
    
    // Lấy giá trị từ JSON (có kiểm tra tồn tại)
    template<typename T>
    T getField(const Json::Value& root, const std::string& field) {
        if (!root.isMember(field)) {
            throw std::runtime_error("Field not found: " + field);
        }
        if constexpr (std::is_same_v<T, std::string>) {
            return root[field].asString();
        } else if constexpr (std::is_same_v<T, double>) {
            return root[field].asDouble();
        } else if constexpr (std::is_same_v<T, int>) {
            return root[field].asInt();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return root[field].asUInt();
        } else if constexpr (std::is_same_v<T, bool>) {
            return root[field].asBool();
        } else {
            return root[field].as<T>();
        }
    }
    
    std::string getString(const Json::Value& root, const std::string& field) {
        return getField<std::string>(root, field);
    }
    
    double getDouble(const Json::Value& root, const std::string& field) {
        return getField<double>(root, field);
    }
    
    int getInt(const Json::Value& root, const std::string& field) {
        return getField<int>(root, field);
    }

    int getUInt(const Json::Value& root, const std::string& field) {
        return getField<uint32_t>(root, field);
    }
    
    bool getBool(const Json::Value& root, const std::string& field) {
        return getField<bool>(root, field);
    }
    
    // Tạo GoTargetOptions với đầy đủ options
    GoTargetOptions createFullOptions() {
        GoTargetOptions opts;
        opts.angle = 1.57;
        opts.method = "forward";
        opts.maxSpeed = 0.5;
        opts.maxWspeed = 1.0;
        opts.maxAcc = 0.3;
        opts.maxWacc = 0.5;
        opts.duration = 1000;
        opts.taskId = "custom_task_123";
        opts.sourceId = "CUSTOM_SOURCE";
        return opts;
    }
    
    // Tạo GoTargetOptions minimal
    GoTargetOptions createMinimalOptions() {
        return GoTargetOptions();
    }
    
    // Kiểm tra JSON script_args cho goToPoint
    void validateGoToPointArgs(const Json::Value& args, 
                               double expectedX, 
                               double expectedY, 
                               double expectedTheta,
                               int expectedBackMode,
                               const std::string& expectedCoord) {
        EXPECT_DOUBLE_EQ(getDouble(args, "x"), expectedX);
        EXPECT_DOUBLE_EQ(getDouble(args, "y"), expectedY);
        EXPECT_DOUBLE_EQ(getDouble(args, "theta"), expectedTheta);
        EXPECT_EQ(getInt(args, "backMode"), expectedBackMode);
        EXPECT_EQ(getString(args, "coordinate"), expectedCoord);
    }
    
    // Data members
    SeerNavigatorCommandBuilder* builder_;
};

// ============================================
// 1. TEST Serial Number Generation
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, NextSerial_StartsAtZero_IncrementsCorrectly) {
    uint16_t prev = 0;
    
    for (int i = 0; i < 10; ++i) {
        auto frame = builder_->pauseNavigation();
        if (i == 0) {
            EXPECT_EQ(frame.serial, 1);
        } else {
            EXPECT_EQ(frame.serial, prev + 1);
        }
        prev = frame.serial;
    }
}

TEST_F(SeerNavigatorCommandBuilderTest, NextSerial_WrapsAroundAt65535) {
    // Force serial về gần max
    for (int i = 0; i < 65534; ++i) {
        builder_->pauseNavigation();
    }
    
    auto frame1 = builder_->pauseNavigation();
    EXPECT_EQ(frame1.serial, 0);
    
    auto frame2 = builder_->pauseNavigation();
    EXPECT_EQ(frame2.serial, 1);
}

TEST_F(SeerNavigatorCommandBuilderTest, NextSerial_WrapAroundOnlyOnce) {
    // Kiểm tra wrap-around chỉ xảy ra 1 lần
    for (int i = 0; i < 65535; ++i) {
        builder_->pauseNavigation();
    }
    
    auto frame1 = builder_->pauseNavigation();
    EXPECT_EQ(frame1.serial, 1);
    
    auto frame2 = builder_->pauseNavigation();
    EXPECT_EQ(frame2.serial, 2);
    
    auto frame3 = builder_->pauseNavigation();
    EXPECT_EQ(frame3.serial, 3);
}

TEST_F(SeerNavigatorCommandBuilderTest, Serial_UniqueAcrossAllCommands) {
    std::vector<uint16_t> serials;
    
    for (int i = 0; i < 50; ++i) {
        auto frame = builder_->pauseNavigation();
        serials.push_back(frame.serial);
    }
    
    // Kiểm tra không trùng lặp
    std::sort(serials.begin(), serials.end());
    auto it = std::unique(serials.begin(), serials.end());
    EXPECT_EQ(it - serials.begin(), serials.size());
}

// ============================================
// 2. TEST Pause/Resume/Cancel Navigation
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, PauseNavigation_ReturnsCorrectFrame) {
    auto frame = builder_->pauseNavigation();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseNavReq));
    EXPECT_TRUE(frame.payload.empty());
    EXPECT_EQ(frame.version, SEER_VERSION_34);
}

TEST_F(SeerNavigatorCommandBuilderTest, PauseNavigation_MultipleCalls_SerialsIncrement) {
    auto frame1 = builder_->pauseNavigation();
    auto frame2 = builder_->pauseNavigation();
    auto frame3 = builder_->pauseNavigation();
    
    EXPECT_EQ(frame2.serial, frame1.serial + 1);
    EXPECT_EQ(frame3.serial, frame2.serial + 1);
}

TEST_F(SeerNavigatorCommandBuilderTest, ResumeNavigation_ReturnsCorrectFrame) {
    auto frame = builder_->resumeNavigation();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::ResumeNavReq));
    EXPECT_TRUE(frame.payload.empty());
}

TEST_F(SeerNavigatorCommandBuilderTest, CancelNavigation_ReturnsCorrectFrame) {
    auto frame = builder_->cancelNavigation();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::CancelNavReq));
    EXPECT_TRUE(frame.payload.empty());
}

TEST_F(SeerNavigatorCommandBuilderTest, NavigationCommands_AllHaveEmptyPayload) {
    auto pause = builder_->pauseNavigation();
    auto resume = builder_->resumeNavigation();
    auto cancel = builder_->cancelNavigation();
    
    EXPECT_TRUE(pause.payload.empty());
    EXPECT_TRUE(resume.payload.empty());
    EXPECT_TRUE(cancel.payload.empty());
}

// ============================================
// 3. TEST GoToStation
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithRequiredFields_ReturnsValidFrame) {
    std::string targetId = "station_1";
    GoTargetOptions opts;
    
    auto frame = builder_->goToStation(targetId, opts);
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetReq));
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "id"), targetId);
    EXPECT_EQ(getString(payload, "source_id"), "SELF_POSITION");
    EXPECT_TRUE(hasField(payload, "task_id"));
    EXPECT_TRUE(getString(payload, "task_id").find("nav_") != std::string::npos);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithAllOptions_IncludesAllFields) {
    auto opts = createFullOptions();
    
    auto frame = builder_->goToStation("station_2", opts);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "angle"), 1.57);
    EXPECT_EQ(getString(payload, "method"), "forward");
    EXPECT_DOUBLE_EQ(getDouble(payload, "max_speed"), 0.5);
    EXPECT_DOUBLE_EQ(getDouble(payload, "max_wspeed"), 1.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "max_acc"), 0.3);
    EXPECT_DOUBLE_EQ(getDouble(payload, "max_wacc"), 0.5);
    EXPECT_EQ(getInt(payload, "duration"), 1000);
    EXPECT_EQ(getString(payload, "task_id"), "custom_task_123");
    EXPECT_EQ(getString(payload, "source_id"), "CUSTOM_SOURCE");
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithCustomSourceId_UsesSourceId) {
    GoTargetOptions opts;
    opts.sourceId = "CUSTOM_SOURCE";
    
    auto frame = builder_->goToStation("station_3", opts);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "source_id"), "CUSTOM_SOURCE");
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithoutTaskId_GeneratesTaskId) {
    auto frame1 = builder_->goToStation("station_1");
    auto frame2 = builder_->goToStation("station_2");
    
    auto payload1 = parsePayload(frame1.payload);
    auto payload2 = parsePayload(frame2.payload);
    
    EXPECT_NE(getString(payload1, "task_id"), getString(payload2, "task_id"));
    EXPECT_TRUE(getString(payload1, "task_id").find("nav_") != std::string::npos);
    EXPECT_TRUE(getString(payload2, "task_id").find("nav_") != std::string::npos);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithOnlyAngle_IncludesAngleOnly) {
    GoTargetOptions opts;
    opts.angle = 2.5;
    
    auto frame = builder_->goToStation("station_4", opts);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "angle"), 2.5);
    EXPECT_FALSE(hasField(payload, "method"));
    EXPECT_FALSE(hasField(payload, "max_speed"));
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithOnlyMethod_IncludesMethodOnly) {
    GoTargetOptions opts;
    opts.method = "backward";
    
    auto frame = builder_->goToStation("station_5", opts);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "method"), "backward");
    EXPECT_FALSE(hasField(payload, "angle"));
    EXPECT_FALSE(hasField(payload, "max_speed"));
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithOnlySpeed_IncludesSpeedOnly) {
    GoTargetOptions opts;
    opts.maxSpeed = 1.5;
    
    auto frame = builder_->goToStation("station_6", opts);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "max_speed"), 1.5);
    EXPECT_FALSE(hasField(payload, "angle"));
    EXPECT_FALSE(hasField(payload, "method"));
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithAllOptional_NoThrow) {
    auto opts = createFullOptions();
    
    EXPECT_NO_THROW({
        auto frame = builder_->goToStation("station", opts);
        EXPECT_FALSE(frame.payload.empty());
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_EmptyTargetId_StillGenerates) {
    auto frame = builder_->goToStation("");
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "id"), "");
    EXPECT_TRUE(hasField(payload, "source_id"));
    EXPECT_TRUE(hasField(payload, "task_id"));
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_WithTaskId_NoGeneration) {
    GoTargetOptions opts;
    opts.taskId = "my_custom_task";
    
    auto frame = builder_->goToStation("station", opts);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "task_id"), "my_custom_task");
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_MultipleStations_DifferentTaskIds) {
    std::vector<std::string> taskIds;
    
    for (int i = 0; i < 10; ++i) {
        auto frame = builder_->goToStation("station_" + std::to_string(i));
        auto payload = parsePayload(frame.payload);
        taskIds.push_back(getString(payload, "task_id"));
    }
    
    // Kiểm tra tất cả taskId khác nhau
    std::sort(taskIds.begin(), taskIds.end());
    auto it = std::unique(taskIds.begin(), taskIds.end());
    EXPECT_EQ(it - taskIds.begin(), taskIds.size());
}

// ============================================
// 4. TEST GoToPoint (Với Enum)
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_ForwardMode_BackModeIs0) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetReq));
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getInt(args, "backMode"), 0);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_BackwardMode_BackModeIs1) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Backward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getInt(args, "backMode"), 1);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_RobotCoordinate_SetsCoordinateRobot) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getString(args, "coordinate"), "robot");
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_WorldCoordinate_SetsCoordinateWorld) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::WORLD,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getString(args, "coordinate"), "world");
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_DefaultParams_ReturnsValidFrame) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    
    EXPECT_DOUBLE_EQ(getDouble(args, "x"), 1.0);
    EXPECT_DOUBLE_EQ(getDouble(args, "y"), 2.0);
    EXPECT_DOUBLE_EQ(getDouble(args, "theta"), 0.5);
    EXPECT_EQ(getInt(args, "backMode"), 0);
    EXPECT_EQ(getString(args, "coordinate"), "robot");
    EXPECT_EQ(getString(payload, "script_name"), "syspy/goPath.py");
    EXPECT_EQ(getString(payload, "operation"), "Script");
    EXPECT_EQ(getString(payload, "id"), "SELF_POSITION");
    EXPECT_EQ(getString(payload, "source_id"), "SELF_POSITION");
    EXPECT_TRUE(getString(payload, "task_id").find("pt_") != std::string::npos);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_WithCustomTaskId_UsesCustomId) {
    std::string customId = "custom_point_task";
    
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    customId);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "task_id"), customId);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_AllCombinations_HandlesCorrectly) {
    std::vector<std::tuple<NavigatorBackMode, NavigatorCoordinate, int, std::string>> testCases = {
        {NavigatorBackMode::Forward, NavigatorCoordinate::SELF, 0, "robot"},
        {NavigatorBackMode::Forward, NavigatorCoordinate::WORLD, 0, "world"},
        {NavigatorBackMode::Backward, NavigatorCoordinate::SELF, 1, "robot"},
        {NavigatorBackMode::Backward, NavigatorCoordinate::WORLD, 1, "world"}
    };
    
    for (const auto& [backMode, coord, expectedBack, expectedCoord] : testCases) {
        auto frame = builder_->goToPoint(1.0, 2.0, 0.5, backMode, coord, "");
        auto payload = parsePayload(frame.payload);
        auto args = payload["script_args"];
        
        EXPECT_EQ(getInt(args, "backMode"), expectedBack) 
            << "Failed for backMode: " << static_cast<int>(backMode);
        EXPECT_EQ(getString(args, "coordinate"), expectedCoord)
            << "Failed for coordinate: " << static_cast<int>(coord);
    }
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_VariousCoordinates_HandlesCorrectly) {
    std::vector<std::tuple<double, double, double>> testCases = {
        {0.0, 0.0, 0.0},
        {-10.5, 20.3, -1.57},
        {100.0, -200.0, 3.14},
        {1e-6, -1e-6, M_PI},
        {-M_PI, M_PI_2, M_PI_4}
    };
    // test gia tri gen ra co giong input dau vao khong
    for (const auto& [x, y, theta] : testCases) {
        auto frame = builder_->goToPoint(x, y, theta, 
                                        NavigatorBackMode::Forward,
                                        NavigatorCoordinate::SELF,
                                        "");
        auto payload = parsePayload(frame.payload);
        auto args = payload["script_args"];
        
        EXPECT_NEAR(getDouble(args, "x"), x, 1e-6);
        EXPECT_NEAR(getDouble(args, "y"), y, 1e-6);
        EXPECT_NEAR(getDouble(args, "theta"), theta, 1e-6);
    }
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_WithoutTaskId_GeneratesTaskId) {
    auto frame1 = builder_->goToPoint(1.0, 2.0, 0.5, 
                                      NavigatorBackMode::Forward,
                                      NavigatorCoordinate::SELF,
                                      "");
    auto frame2 = builder_->goToPoint(3.0, 4.0, 1.0, 
                                      NavigatorBackMode::Forward,
                                      NavigatorCoordinate::SELF,
                                      "");
    
    auto payload1 = parsePayload(frame1.payload);
    auto payload2 = parsePayload(frame2.payload);
    
    EXPECT_NE(getString(payload1, "task_id"), getString(payload2, "task_id"));
    EXPECT_TRUE(getString(payload1, "task_id").find("pt_") != std::string::npos);
    EXPECT_TRUE(getString(payload2, "task_id").find("pt_") != std::string::npos);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_WithEmptyTaskId_GeneratesNewTaskId) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    EXPECT_TRUE(getString(payload, "task_id").find("pt_") != std::string::npos);
}

// ============================================
// 5. TEST StatusAll1
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_DefaultParams_ReturnsValidFrame) {
    auto frame = builder_->statusAll1();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::StatusAll1Req));
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getBool(payload, "return_laser"), false);
    EXPECT_EQ(getBool(payload, "return_beams3D"), false);
    EXPECT_EQ(getInt(payload, "timeout"), 500);
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_WithCustomParams_SetsCorrectly) {
    auto frame = builder_->statusAll1(true, true, 1000);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getBool(payload, "return_laser"), true);
    EXPECT_EQ(getBool(payload, "return_beams3D"), true);
    EXPECT_EQ(getInt(payload, "timeout"), 1000);
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_ReturnLaserOnly_ReturnBeamsFalse) {
    auto frame = builder_->statusAll1(true, false, 500);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getBool(payload, "return_laser"), true);
    EXPECT_EQ(getBool(payload, "return_beams3D"), false);
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_ReturnBeamsOnly_ReturnLaserFalse) {
    auto frame = builder_->statusAll1(false, true, 500);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getBool(payload, "return_laser"), false);
    EXPECT_EQ(getBool(payload, "return_beams3D"), true);
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_WithTimeoutZero_SetsZero) {
    auto frame = builder_->statusAll1(false, false, 0);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getInt(payload, "timeout"), 0);
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_WithLargeTimeout_HandlesCorrectly) {
    auto frame = builder_->statusAll1(false, false, 999999);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getInt(payload, "timeout"), 999999);
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_WithAllParamsTrue_ReturnsCorrect) {
    auto frame = builder_->statusAll1(true, true, 2000);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getBool(payload, "return_laser"), true);
    EXPECT_EQ(getBool(payload, "return_beams3D"), true);
    EXPECT_EQ(getInt(payload, "timeout"), 2000);
}

// ============================================
// 6. TEST Relocation
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, Relocation_ValidParams_ReturnsValidFrame) {
    auto frame = builder_->relocation(1.5, 2.5, 1.57);
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::RelocationReq));
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "x"), 1.5);
    EXPECT_DOUBLE_EQ(getDouble(payload, "y"), 2.5);
    EXPECT_DOUBLE_EQ(getDouble(payload, "angle"), 1.57);
    EXPECT_DOUBLE_EQ(getDouble(payload, "length"), 1.0);
}

TEST_F(SeerNavigatorCommandBuilderTest, Relocation_WithNegativeCoordinates_HandlesCorrectly) {
    auto frame = builder_->relocation(-10.0, -20.0, -3.14);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "x"), -10.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "y"), -20.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "angle"), -3.14);
}

TEST_F(SeerNavigatorCommandBuilderTest, Relocation_WithZeroCoordinates_HandlesCorrectly) {
    auto frame = builder_->relocation(0.0, 0.0, 0.0);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "x"), 0.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "y"), 0.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "angle"), 0.0);
}

TEST_F(SeerNavigatorCommandBuilderTest, Relocation_WithLargeCoordinates_HandlesCorrectly) {
    // check output sinh ra co giong input nhap vao
    auto frame = builder_->relocation(1e6, -1e6, 2*M_PI);
    auto payload = parsePayload(frame.payload);
    EXPECT_NEAR(getDouble(payload, "x"), 1e6, 1e-6);       // x = 1,000,000
    EXPECT_NEAR(getDouble(payload, "y"), -1e6, 1e-6);      // y = -1,000,000
    EXPECT_NEAR(getDouble(payload, "angle"), 2*M_PI, 1e-6); // angle = 2π
}

TEST_F(SeerNavigatorCommandBuilderTest, ConfirmLocation_ReturnsValidFrame) {
    auto frame = builder_->confirmLocation();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::ConfirmCorrectRelocationReq));
    EXPECT_TRUE(frame.payload.empty());
}

TEST_F(SeerNavigatorCommandBuilderTest, Relocation_LengthAlwaysOne_TestMultiple) {
    for (int i = 0; i < 10; ++i) {
        auto frame = builder_->relocation(i, i*2, i*0.1);
        auto payload = parsePayload(frame.payload);
        EXPECT_DOUBLE_EQ(getDouble(payload, "length"), 1.0);
    }
}

// ============================================
// 7. TEST OpenLoopMotion
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, OpenLoopMotion_ValidParams_ReturnsValidFrame) {
    auto frame = builder_->openLoopMotion(0.5, 0.1, 0.2, 1000);
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::OpenLoopMotionReq));
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "vx"), 0.5);
    EXPECT_DOUBLE_EQ(getDouble(payload, "vy"), 0.1);
    EXPECT_DOUBLE_EQ(getDouble(payload, "w"), 0.2);
    EXPECT_EQ(getInt(payload, "duration"), 1000);
}

TEST_F(SeerNavigatorCommandBuilderTest, OpenLoopMotion_ZeroValues_HandlesCorrectly) {
    auto frame = builder_->openLoopMotion(0.0, 0.0, 0.0, 0);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "vx"), 0.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "vy"), 0.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "w"), 0.0);
    EXPECT_EQ(getInt(payload, "duration"), 0);
}

TEST_F(SeerNavigatorCommandBuilderTest, OpenLoopMotion_LargeDuration_HandlesCorrectly) {
    uint32_t maxDuration = 0xFFFFFFFF;
    auto frame = builder_->openLoopMotion(0.1, 0.2, 0.3, maxDuration);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getUInt (payload, "duration"), static_cast<uint32_t>(maxDuration));
}

TEST_F(SeerNavigatorCommandBuilderTest, OpenLoopMotion_VariousValues_HandlesCorrectly) {
    std::vector<std::tuple<double, double, double, uint32_t>> testCases = {
        {0.1, 0.2, 0.3, 100},
        {1.0, 2.0, 3.0, 1000},
        {-1.0, -2.0, -3.0, 10000},
        {0.01, 0.02, 0.03, 1}
    };
    
    for (const auto& [vx, vy, w, duration] : testCases) {
        auto frame = builder_->openLoopMotion(vx, vy, w, duration);
        auto payload = parsePayload(frame.payload);
        
        EXPECT_DOUBLE_EQ(getDouble(payload, "vx"), vx);
        EXPECT_DOUBLE_EQ(getDouble(payload, "vy"), vy);
        EXPECT_DOUBLE_EQ(getDouble(payload, "w"), w);
        EXPECT_EQ(getInt(payload, "duration"), static_cast<int>(duration));
    }
}

// ============================================
// 8. TEST Audio Commands
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, PlayAudio_ValidName_ReturnsValidFrame) {
    auto frame = builder_->playAudio("collision");
    
    validateBasicFrame(frame, static_cast<uint16_t>(SeerNavigatorMessageNumber::PlayAudioReq));
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "name"), "collision");
    EXPECT_EQ(getBool(payload, "loop"), true);
}

TEST_F(SeerNavigatorCommandBuilderTest, PlayAudio_WithDifferentName_SetsCorrectName) {
    auto frame = builder_->playAudio("alarm");
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "name"), "alarm");
}

TEST_F(SeerNavigatorCommandBuilderTest, PlayAudio_WithSpecialCharacters_HandlesCorrectly) {
    std::vector<std::string> testNames = {
        "audio_1_2_3",
        "audio@#$%",
        "audio-with-dash",
        "audio_with_underscore",
        "audio.with.dot"
    };
    
    for (const auto& name : testNames) {
        auto frame = builder_->playAudio(name);
        auto payload = parsePayload(frame.payload);
        EXPECT_EQ(getString(payload, "name"), name);
    }
}

TEST_F(SeerNavigatorCommandBuilderTest, PlayAudio_WithEmptyName_HandlesEmptyString) {
    auto frame = builder_->playAudio("");
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "name"), "");
}

TEST_F(SeerNavigatorCommandBuilderTest, PlayAudio_LoopAlwaysTrue) {
    auto frame1 = builder_->playAudio("test1");
    auto frame2 = builder_->playAudio("test2");
    
    auto payload1 = parsePayload(frame1.payload);
    auto payload2 = parsePayload(frame2.payload);
    
    EXPECT_EQ(getBool(payload1, "loop"), true);
    EXPECT_EQ(getBool(payload2, "loop"), true);
}

TEST_F(SeerNavigatorCommandBuilderTest, StopAudio_ReturnsValidFrame) {
    auto frame = builder_->stopAudio();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::StopAudioReq));
    EXPECT_TRUE(frame.payload.empty());
}

TEST_F(SeerNavigatorCommandBuilderTest, PauseAudio_ReturnsValidFrame) {
    auto frame = builder_->pauseAudio();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseAudioReq));
    EXPECT_TRUE(frame.payload.empty());
}

TEST_F(SeerNavigatorCommandBuilderTest, ResumeAudio_ReturnsValidFrame) {
    auto frame = builder_->resumeAudio();
    
    validateBasicFrame(frame, 
        static_cast<uint16_t>(SeerNavigatorMessageNumber::ResumeAudioReq));
    EXPECT_TRUE(frame.payload.empty());
}

// ============================================
// 9. TEST Integration with Codec
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, Integration_EncodeDecodePauseCommand_PreservesData) {
    auto frame = builder_->pauseNavigation();
    
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->version, frame.version);
    EXPECT_EQ(decoded->serial, frame.serial);
    EXPECT_EQ(decoded->msgType, frame.msgType);
    EXPECT_EQ(decoded->payload, frame.payload);
}

TEST_F(SeerNavigatorCommandBuilderTest, Integration_EncodeDecodeGoToStation_PreservesData) {
    auto frame = builder_->goToStation("station_1");
    
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->version, frame.version);
    EXPECT_EQ(decoded->serial, frame.serial);
    EXPECT_EQ(decoded->msgType, frame.msgType);
    EXPECT_EQ(decoded->payload, frame.payload);
}

TEST_F(SeerNavigatorCommandBuilderTest, Integration_EncodeDecodeGoToPoint_PreservesData) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "task_123");
    
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->serial, frame.serial);
    EXPECT_EQ(decoded->msgType, frame.msgType);
    EXPECT_EQ(decoded->payload, frame.payload);
}

TEST_F(SeerNavigatorCommandBuilderTest, Integration_EncodeDecodeStatusAll1_PreservesData) {
    auto frame = builder_->statusAll1(true, false, 500);
    
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->serial, frame.serial);
    EXPECT_EQ(decoded->msgType, frame.msgType);
    EXPECT_EQ(decoded->payload, frame.payload);
}

TEST_F(SeerNavigatorCommandBuilderTest, Integration_EncodeDecodeRelocation_PreservesData) {
    auto frame = builder_->relocation(1.0, 2.0, 0.5);
    
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->serial, frame.serial);
    EXPECT_EQ(decoded->msgType, frame.msgType);
    EXPECT_EQ(decoded->payload, frame.payload);
}

TEST_F(SeerNavigatorCommandBuilderTest, Integration_EncodeDecodeOpenLoopMotion_PreservesData) {
    auto frame = builder_->openLoopMotion(0.1, 0.2, 0.3, 500);
    
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->serial, frame.serial);
    EXPECT_EQ(decoded->msgType, frame.msgType);
    EXPECT_EQ(decoded->payload, frame.payload);
}

TEST_F(SeerNavigatorCommandBuilderTest, Integration_EncodeDecodePlayAudio_PreservesData) {
    auto frame = builder_->playAudio("test_audio");
    
    auto encoded = SeerNavigatorFrameCodec::encode(frame);
    auto decoded = SeerNavigatorFrameCodec::decode(encoded);
    
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->serial, frame.serial);
    EXPECT_EQ(decoded->msgType, frame.msgType);
    EXPECT_EQ(decoded->payload, frame.payload);
}

TEST_F(SeerNavigatorCommandBuilderTest, Integration_MultipleCommands_AllEncodable) {
    std::vector<SeerNavigatorFrame> frames;
    frames.push_back(builder_->pauseNavigation());
    frames.push_back(builder_->resumeNavigation());
    frames.push_back(builder_->cancelNavigation());
    frames.push_back(builder_->goToStation("station_1"));
    frames.push_back(builder_->goToPoint(1.0, 2.0, 0.5, 
                                        NavigatorBackMode::Forward,
                                        NavigatorCoordinate::SELF,
                                        ""));
    frames.push_back(builder_->statusAll1(true, false, 1000));
    frames.push_back(builder_->playAudio("test"));
    frames.push_back(builder_->relocation(1.0, 2.0, 0.5));
    frames.push_back(builder_->confirmLocation());
    frames.push_back(builder_->openLoopMotion(0.1, 0.2, 0.3, 500));
    frames.push_back(builder_->stopAudio());
    frames.push_back(builder_->pauseAudio());
    frames.push_back(builder_->resumeAudio());
    
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
// 10. TEST JSON Format Validation
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, Json_GoToPoint_GeneratesValidJson) {
    auto frame = builder_->goToPoint(1.5, 2.5, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    EXPECT_NO_THROW({
        auto payload = parsePayload(frame.payload);
        EXPECT_TRUE(payload.isObject());
        EXPECT_TRUE(hasField(payload, "script_name"));
        EXPECT_TRUE(hasField(payload, "script_args"));
        EXPECT_TRUE(hasField(payload, "operation"));
        EXPECT_TRUE(hasField(payload, "id"));
        EXPECT_TRUE(hasField(payload, "source_id"));
        EXPECT_TRUE(hasField(payload, "task_id"));
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, Json_GoToStation_GeneratesValidJson) {
    GoTargetOptions opts;
    opts.angle = 1.57;
    auto frame = builder_->goToStation("test", opts);
    
    EXPECT_NO_THROW({
        auto payload = parsePayload(frame.payload);
        EXPECT_TRUE(payload.isObject());
        EXPECT_TRUE(hasField(payload, "id"));
        EXPECT_TRUE(hasField(payload, "source_id"));
        EXPECT_TRUE(hasField(payload, "task_id"));
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, Json_StatusAll1_GeneratesValidJson) {
    auto frame = builder_->statusAll1(true, false, 500);
    
    EXPECT_NO_THROW({
        auto payload = parsePayload(frame.payload);
        EXPECT_TRUE(payload.isObject());
        EXPECT_TRUE(hasField(payload, "return_laser"));
        EXPECT_TRUE(hasField(payload, "return_beams3D"));
        EXPECT_TRUE(hasField(payload, "timeout"));
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, Json_Relocation_GeneratesValidJson) {
    auto frame = builder_->relocation(1.0, 2.0, 0.5);
    
    EXPECT_NO_THROW({
        auto payload = parsePayload(frame.payload);
        EXPECT_TRUE(payload.isObject());
        EXPECT_TRUE(hasField(payload, "x"));
        EXPECT_TRUE(hasField(payload, "y"));
        EXPECT_TRUE(hasField(payload, "angle"));
        EXPECT_TRUE(hasField(payload, "length"));
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, Json_OpenLoopMotion_GeneratesValidJson) {
    auto frame = builder_->openLoopMotion(0.1, 0.2, 0.3, 500);
    
    EXPECT_NO_THROW({
        auto payload = parsePayload(frame.payload);
        EXPECT_TRUE(payload.isObject());
        EXPECT_TRUE(hasField(payload, "vx"));
        EXPECT_TRUE(hasField(payload, "vy"));
        EXPECT_TRUE(hasField(payload, "w"));
        EXPECT_TRUE(hasField(payload, "duration"));
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, Json_PlayAudio_GeneratesValidJson) {
    auto frame = builder_->playAudio("test_audio");
    
    EXPECT_NO_THROW({
        auto payload = parsePayload(frame.payload);
        EXPECT_TRUE(payload.isObject());
        EXPECT_TRUE(hasField(payload, "name"));
        EXPECT_TRUE(hasField(payload, "loop"));
    });
}

// ============================================
// 11. TEST Edge Cases - Invalid Input
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_SpecialCharactersInTargetId_HandlesCorrectly) {
    std::vector<std::string> testCases = {
        "station_1",
        "station@#$%",
        "12345",
        "   ",
        "station-with-dash",
        "station_with_underscore",
        "station.with.dot",
        "station:with:colon",
        "station/with/slash"
    };
    
    for (const auto& targetId : testCases) {
        auto frame = builder_->goToStation(targetId);
        auto payload = parsePayload(frame.payload);
        EXPECT_EQ(getString(payload, "id"), targetId);
    }
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_LongTargetId_HandlesCorrectly) {
    std::string longTargetId(1000, 'A');
    auto frame = builder_->goToStation(longTargetId);
    
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "id"), longTargetId);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_ExtremelyLongTargetId_HandlesCorrectly) {
    std::string longTargetId(10000, 'B');
    EXPECT_NO_THROW({
        auto frame = builder_->goToStation(longTargetId);
        auto payload = parsePayload(frame.payload);
        EXPECT_EQ(getString(payload, "id"), longTargetId);
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_NanCoordinates_HandlesGracefully) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    
    EXPECT_NO_THROW({
        auto frame = builder_->goToPoint(nan, 1.0, 1.0, 
                                        NavigatorBackMode::Forward,
                                        NavigatorCoordinate::SELF,
                                        "");
        // Không assert giá trị NaN vì NaN != NaN
        // Chỉ đảm bảo không crash
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_InvalidMethod_StillGenerates) {
    GoTargetOptions opts;
    opts.method = "invalid_method_123!@#";
    
    auto frame = builder_->goToStation("station", opts);
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "method"), "invalid_method_123!@#");
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_NegativeSpeed_StillGenerates) {
    GoTargetOptions opts;
    opts.maxSpeed = -1.0;
    opts.maxWspeed = -2.0;
    
    auto frame = builder_->goToStation("station", opts);
    auto payload = parsePayload(frame.payload);
    EXPECT_DOUBLE_EQ(getDouble(payload, "max_speed"), -1.0);
    EXPECT_DOUBLE_EQ(getDouble(payload, "max_wspeed"), -2.0);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_NegativeDuration_StillGenerates) {
    GoTargetOptions opts;
    opts.duration = -1000;
    
    auto frame = builder_->goToStation("station", opts);
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getInt(payload, "duration"), -1000);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_LargeAngle_HandlesCorrectly) {
    double angle = 100.0 * M_PI;
    auto frame = builder_->goToPoint(1.0, 2.0, angle, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_NEAR(getDouble(args, "theta"), angle, 1e-6);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToPoint_SpecialCharsInTaskId_HandlesCorrectly) {
    std::vector<std::string> testCases = {
        "task_123",
        "task@#$%",
        "task with spaces",
        "task-with-dash",
        "task_with_underscore",
        "task.with.dot",
        "task:with:colon",
        "task/with/slash"
    };
    
    for (const auto& taskId : testCases) {
        auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                        NavigatorBackMode::Forward,
                                        NavigatorCoordinate::SELF,
                                        taskId);
        auto payload = parsePayload(frame.payload);
        EXPECT_EQ(getString(payload, "task_id"), taskId);
    }
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_InvalidTimeout_StillGenerates) {
    // Timeout có thể là số âm hoặc quá lớn
    auto frame = builder_->statusAll1(false, false, -100);
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getInt(payload, "timeout"), -100);
}

TEST_F(SeerNavigatorCommandBuilderTest, StatusAll1_VeryLargeTimeout_HandlesCorrectly) {
    auto frame = builder_->statusAll1(false, false, 999999999);
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getInt(payload, "timeout"), 999999999);
}

TEST_F(SeerNavigatorCommandBuilderTest, PlayAudio_VeryLongName_HandlesCorrectly) {
    std::string longName(10000, 'A');
    auto frame = builder_->playAudio(longName);
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "name"), longName);
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_EmptyOpts_GeneratesMinimalJson) {
    GoTargetOptions opts;  // Default
    auto frame = builder_->goToStation("station", opts);
    auto payload = parsePayload(frame.payload);
    
    // Chỉ có 3 field bắt buộc
    EXPECT_EQ(payload.size(), 3);
    EXPECT_TRUE(hasField(payload, "id"));
    EXPECT_TRUE(hasField(payload, "source_id"));
    EXPECT_TRUE(hasField(payload, "task_id"));
    EXPECT_FALSE(hasField(payload, "angle"));
    EXPECT_FALSE(hasField(payload, "method"));
    EXPECT_FALSE(hasField(payload, "max_speed"));
}

TEST_F(SeerNavigatorCommandBuilderTest, GoToStation_LongSourceId_HandlesCorrectly) {
    GoTargetOptions opts;
    opts.sourceId = std::string(500, 'S');
    
    auto frame = builder_->goToStation("station", opts);
    auto payload = parsePayload(frame.payload);
    EXPECT_EQ(getString(payload, "source_id"), opts.sourceId);
}

// ============================================
// 12. TEST Enum Value Mapping
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, EnumBackMode_ForwardMapsTo0) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getInt(args, "backMode"), 0);
}

TEST_F(SeerNavigatorCommandBuilderTest, EnumBackMode_BackwardMapsTo1) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Backward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getInt(args, "backMode"), 1);
}

TEST_F(SeerNavigatorCommandBuilderTest, EnumCoordinate_RobotMapsToString) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::SELF,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getString(args, "coordinate"), "robot");
}

TEST_F(SeerNavigatorCommandBuilderTest, EnumCoordinate_WorldMapsToString) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                    NavigatorBackMode::Forward,
                                    NavigatorCoordinate::WORLD,
                                    "");
    
    auto payload = parsePayload(frame.payload);
    auto args = payload["script_args"];
    EXPECT_EQ(getString(args, "coordinate"), "world");
}

// ============================================
// 13. TEST Error Safety & Performance
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, ErrorHandling_MixedCommands_NoThrow) {
    EXPECT_NO_THROW({
        // Gọi tất cả các command liên tục
        builder_->pauseNavigation();
        builder_->resumeNavigation();
        builder_->cancelNavigation();
        builder_->goToStation("station_1");
        builder_->goToPoint(1.0, 2.0, 0.5, 
                           NavigatorBackMode::Forward,
                           NavigatorCoordinate::SELF,
                           "task1");
        builder_->statusAll1(true, false, 500);
        builder_->relocation(1.0, 2.0, 0.5);
        builder_->confirmLocation();
        builder_->openLoopMotion(0.1, 0.2, 0.3, 1000);
        builder_->playAudio("test");
        builder_->stopAudio();
        builder_->pauseAudio();
        builder_->resumeAudio();
    });
}

TEST_F(SeerNavigatorCommandBuilderTest, ExceptionSafety_AfterInvalidInput_ContinuesWorking) {
    // Gọi với input bất thường (không throw)
    EXPECT_NO_THROW({
        builder_->goToPoint(std::numeric_limits<double>::quiet_NaN(), 
                           1.0, 1.0, 
                           NavigatorBackMode::Forward,
                           NavigatorCoordinate::SELF,
                           "");
    });
    
    // Builder vẫn hoạt động bình thường
    auto frame = builder_->pauseNavigation();
    EXPECT_GT(frame.serial, 0);
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseNavReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, ErrorHandling_ManyCommands_NoCrash) {
    for (int i = 0; i < 100; ++i) {
        auto frame1 = builder_->goToStation("station_" + std::to_string(i));
        auto frame2 = builder_->goToPoint(i, i*2, i*0.1, 
                                          NavigatorBackMode::Forward,
                                          NavigatorCoordinate::SELF,
                                          "task_" + std::to_string(i));
        auto frame3 = builder_->statusAll1(i % 2 == 0, i % 3 == 0, i * 10);
        auto frame4 = builder_->playAudio("audio_" + std::to_string(i));
        
        // Verify tất cả đều có serial tăng dần
        EXPECT_GT(frame1.serial, 0);
        EXPECT_GT(frame2.serial, frame1.serial);
        EXPECT_GT(frame3.serial, frame2.serial);
        EXPECT_GT(frame4.serial, frame3.serial);
    }
}

TEST_F(SeerNavigatorCommandBuilderTest, MemorySafety_ManyCommands_NoLeak) {
    // Tạo 10000 command
    for (int i = 0; i < 10000; ++i) {
        auto frame = builder_->goToStation("station_" + std::to_string(i));
        EXPECT_FALSE(frame.payload.empty());
    }
    
    // Không assert gì thêm, chỉ cần không crash
    SUCCEED();
}

TEST_F(SeerNavigatorCommandBuilderTest, Performance_RapidCommands_SerialIncrements) {
    std::vector<uint16_t> serials;
    
    for (int i = 0; i < 10000; ++i) {
        auto frame = builder_->pauseNavigation();
        serials.push_back(frame.serial);
    }
    
    // Kiểm tra tất cả serial đều tăng dần
    for (size_t i = 1; i < serials.size(); ++i) {
        EXPECT_EQ(serials[i], serials[i-1] + 1);
    }
}

// ============================================
// 14. TEST Message Type Mapping
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_PauseNavigation_Correct) {
    auto frame = builder_->pauseNavigation();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseNavReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_ResumeNavigation_Correct) {
    auto frame = builder_->resumeNavigation();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::ResumeNavReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_CancelNavigation_Correct) {
    auto frame = builder_->cancelNavigation();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::CancelNavReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_GoToStation_Correct) {
    auto frame = builder_->goToStation("station");
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_GoToPoint_Correct) {
    auto frame = builder_->goToPoint(1.0, 2.0, 0.5, 
                                     NavigatorBackMode::Forward,
                                     NavigatorCoordinate::SELF,
                                     "");
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_StatusAll1_Correct) {
    auto frame = builder_->statusAll1();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::StatusAll1Req));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_Relocation_Correct) {
    auto frame = builder_->relocation(1.0, 2.0, 0.5);
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::RelocationReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_ConfirmLocation_Correct) {
    auto frame = builder_->confirmLocation();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::ConfirmCorrectRelocationReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_OpenLoopMotion_Correct) {
    auto frame = builder_->openLoopMotion(0.1, 0.2, 0.3, 500);
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::OpenLoopMotionReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_PlayAudio_Correct) {
    auto frame = builder_->playAudio("test");
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::PlayAudioReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_StopAudio_Correct) {
    auto frame = builder_->stopAudio();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::StopAudioReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_PauseAudio_Correct) {
    auto frame = builder_->pauseAudio();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseAudioReq));
}

TEST_F(SeerNavigatorCommandBuilderTest, MessageType_ResumeAudio_Correct) {
    auto frame = builder_->resumeAudio();
    EXPECT_EQ(frame.msgType, static_cast<uint16_t>(SeerNavigatorMessageNumber::ResumeAudioReq));
}

// ============================================
// 15. TEST Version Number
// ============================================

TEST_F(SeerNavigatorCommandBuilderTest, Version_AllCommands_UseVersion34) {
    std::vector<SeerNavigatorFrame> frames;
    frames.push_back(builder_->pauseNavigation());
    frames.push_back(builder_->goToStation("station"));
    frames.push_back(builder_->goToPoint(1.0, 2.0, 0.5, 
                                        NavigatorBackMode::Forward,
                                        NavigatorCoordinate::SELF,
                                        ""));
    frames.push_back(builder_->statusAll1());
    frames.push_back(builder_->relocation(1.0, 2.0, 0.5));
    frames.push_back(builder_->openLoopMotion(0.1, 0.2, 0.3, 500));
    frames.push_back(builder_->playAudio("test"));
    
    for (const auto& frame : frames) {
        EXPECT_EQ(frame.version, SEER_VERSION_34);
    }
}