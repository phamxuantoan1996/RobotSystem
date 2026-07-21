#include <cmath>
#include <gtest/gtest.h>
#include "velocity.hpp"
#include <type_traits>

using namespace navigator::domain::value_objects;

// ============================================
// Test Fixture cho Velocity
// ============================================
class VelocityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Khởi tạo các giá trị mặc định
        default_vx = 1.5;
        default_vy = 2.3;
        default_vw = 0.5;
    }

    double default_vx;
    double default_vy;
    double default_vw;
};

// ============================================
// 1. Test Constructor và khởi tạo
// ============================================

TEST_F(VelocityTest, Constructor_WithValidValues_ShouldCreateObject) {
    // Act
    Velocity vel(default_vx, default_vy, default_vw);

    // Assert
    EXPECT_DOUBLE_EQ(vel.getVx(), default_vx);
    EXPECT_DOUBLE_EQ(vel.getVy(), default_vy);
    EXPECT_DOUBLE_EQ(vel.getVw(), default_vw);
}

TEST_F(VelocityTest, Constructor_WithZeroValues_ShouldCreateObject) {
    // Act
    Velocity vel(0.0, 0.0, 0.0);

    // Assert
    EXPECT_DOUBLE_EQ(vel.getVx(), 0.0);
    EXPECT_DOUBLE_EQ(vel.getVy(), 0.0);
    EXPECT_DOUBLE_EQ(vel.getVw(), 0.0);
}

TEST_F(VelocityTest, Constructor_WithNegativeValues_ShouldCreateObject) {
    // Act
    Velocity vel(-1.5, -2.3, -0.5);

    // Assert
    EXPECT_DOUBLE_EQ(vel.getVx(), -1.5);
    EXPECT_DOUBLE_EQ(vel.getVy(), -2.3);
    EXPECT_DOUBLE_EQ(vel.getVw(), -0.5);
}

TEST_F(VelocityTest, Constructor_WithLargeValues_ShouldCreateObject) {
    // Act
    Velocity vel(1000.5, 2000.3, 999.999);

    // Assert
    EXPECT_DOUBLE_EQ(vel.getVx(), 1000.5);
    EXPECT_DOUBLE_EQ(vel.getVy(), 2000.3);
    EXPECT_DOUBLE_EQ(vel.getVw(), 999.999);
}

TEST_F(VelocityTest, Constructor_WithVerySmallValues_ShouldCreateObject) {
    // Act
    Velocity vel(1e-10, 1e-10, 1e-10);

    // Assert
    EXPECT_DOUBLE_EQ(vel.getVx(), 1e-10);
    EXPECT_DOUBLE_EQ(vel.getVy(), 1e-10);
    EXPECT_DOUBLE_EQ(vel.getVw(), 1e-10);
}

// ============================================
// 2. Test Copy Constructor
// ============================================

TEST_F(VelocityTest, CopyConstructor_ShouldCreateExactCopy) {
    // Arrange
    Velocity original(default_vx, default_vy, default_vw);

    // Act
    Velocity copy(original);

    // Assert
    EXPECT_DOUBLE_EQ(copy.getVx(), original.getVx());
    EXPECT_DOUBLE_EQ(copy.getVy(), original.getVy());
    EXPECT_DOUBLE_EQ(copy.getVw(), original.getVw());
}

TEST_F(VelocityTest, CopyConstructor_NewObjectShouldBeIndependent) {
    // Arrange
    Velocity original(default_vx, default_vy, default_vw);
    
    // Act
    Velocity copy(original);
    
    // Assert - cả hai đều không thể thay đổi (const members)
    // nhưng chúng vẫn giữ giá trị đúng
    EXPECT_DOUBLE_EQ(copy.getVx(), default_vx);
    EXPECT_DOUBLE_EQ(copy.getVy(), default_vy);
    EXPECT_DOUBLE_EQ(copy.getVw(), default_vw);
}

// ============================================
// 3. Test Copy Assignment - Bị delete
// ============================================

TEST_F(VelocityTest, CopyAssignment_ShouldBeDeleted) {
    // Kiểm tra tại compile-time rằng operator= bị delete
    static_assert(!std::is_copy_assignable<Velocity>::value, 
                  "Velocity should not be copy assignable");
    static_assert(!std::is_move_assignable<Velocity>::value, 
                  "Velocity should not be move assignable");
}

// ============================================
// 4. Test Move Constructor - Bị delete
// ============================================

TEST_F(VelocityTest, MoveConstructor_ShouldBeDeleted) {
    static_assert(!std::is_move_constructible<Velocity>::value, 
                  "Velocity should not be move constructible");
}

// ============================================
// 6. Test Getter methods
// ============================================

TEST_F(VelocityTest, GetVx_ShouldReturnCorrectValue) {
    // Arrange
    Velocity vel(default_vx, default_vy, default_vw);

    // Act
    double vx = vel.getVx();

    // Assert
    EXPECT_DOUBLE_EQ(vx, default_vx);
}

TEST_F(VelocityTest, GetVy_ShouldReturnCorrectValue) {
    // Arrange
    Velocity vel(default_vx, default_vy, default_vw);

    // Act
    double vy = vel.getVy();

    // Assert
    EXPECT_DOUBLE_EQ(vy, default_vy);
}

TEST_F(VelocityTest, GetVw_ShouldReturnCorrectValue) {
    // Arrange
    Velocity vel(default_vx, default_vy, default_vw);

    // Act
    double vw = vel.getVw();

    // Assert
    EXPECT_DOUBLE_EQ(vw, default_vw);
}

// ============================================
// 7. Test tính bất biến (Immutability)
// ============================================

TEST_F(VelocityTest, Immutability_ValuesCannotBeChangedAfterConstruction) {
    // Arrange
    Velocity vel(default_vx, default_vy, default_vw);
    
    // Lưu giá trị ban đầu
    double original_vx = vel.getVx();
    double original_vy = vel.getVy();
    double original_vw = vel.getVw();
    
    // Act - Không có cách nào để thay đổi vì tất cả đều const
    // Không thể gán, không có setter
    
    // Assert - Giá trị không đổi
    EXPECT_DOUBLE_EQ(vel.getVx(), original_vx);
    EXPECT_DOUBLE_EQ(vel.getVy(), original_vy);
    EXPECT_DOUBLE_EQ(vel.getVw(), original_vw);
}

// ============================================
// 8. Test với nhiều giá trị khác nhau (Parameterized)
// ============================================

class VelocityParameterizedTest : public ::testing::TestWithParam<std::tuple<double, double, double>> {};

TEST_P(VelocityParameterizedTest, Constructor_WithVariousValues_ShouldCreateObject) {
    // Arrange
    auto [vx, vy, vw] = GetParam();

    // Act
    Velocity vel(vx, vy, vw);

    // Assert
    EXPECT_DOUBLE_EQ(vel.getVx(), vx);
    EXPECT_DOUBLE_EQ(vel.getVy(), vy);
    EXPECT_DOUBLE_EQ(vel.getVw(), vw);
}

INSTANTIATE_TEST_SUITE_P(
    VelocityTests,
    VelocityParameterizedTest,
    ::testing::Values(
        std::make_tuple(0.0, 0.0, 0.0),
        std::make_tuple(1.5, 2.3, 0.5),
        std::make_tuple(-1.5, -2.3, -0.5),
        std::make_tuple(100.0, 200.0, 50.0),
        std::make_tuple(1.234, 5.678, 9.101),
        std::make_tuple(999.999, 888.888, 777.777),
        std::make_tuple(0.1, 0.2, 0.3),
        std::make_tuple(-0.1, -0.2, -0.3)
    )
);

// ============================================
// 9. Test object size và alignment
// ============================================

TEST_F(VelocityTest, ObjectSize_ShouldBeReasonable) {
    // 3 double = 24 bytes (thường)
    size_t expected_min_size = sizeof(double) * 3;
    size_t actual_size = sizeof(Velocity);
    
    // Cho phép padding tối đa 8 bytes
    EXPECT_GE(actual_size, expected_min_size);
    EXPECT_LE(actual_size, expected_min_size + 8);
    
    // In ra kích thước để tham khảo
    std::cout << "Size of Velocity: " << actual_size << " bytes" << std::endl;
}

// ============================================
// 10. Test constructor với giá trị đặc biệt
// ============================================

TEST_F(VelocityTest, Constructor_WithInfinity_ShouldCreateObject) {
    // Act
    Velocity vel(INFINITY, INFINITY, INFINITY);

    // Assert
    EXPECT_TRUE(std::isinf(vel.getVx()));
    EXPECT_TRUE(std::isinf(vel.getVy()));
    EXPECT_TRUE(std::isinf(vel.getVw()));
}

TEST_F(VelocityTest, Constructor_WithNaN_ShouldCreateObject) {
    // Act
    Velocity vel(NAN, NAN, NAN);

    // Assert
    EXPECT_TRUE(std::isnan(vel.getVx()));
    EXPECT_TRUE(std::isnan(vel.getVy()));
    EXPECT_TRUE(std::isnan(vel.getVw()));
}

// ============================================
// 11. Test với combination values
// ============================================

TEST_F(VelocityTest, Combination_AllDifferentValues_ShouldWork) {
    // Arrange
    std::vector<std::tuple<double, double, double>> test_data = {
        {1.0, 2.0, 3.0},
        {-1.0, 2.0, -3.0},
        {0.0, -2.0, 3.0},
        {1.0, 0.0, -3.0},
        {1e5, -2e5, 3e5},
        {-1e-5, 2e-5, -3e-5}
    };
    
    for (const auto& [vx, vy, vw] : test_data) {
        // Act
        Velocity vel(vx, vy, vw);
        
        // Assert
        EXPECT_DOUBLE_EQ(vel.getVx(), vx);
        EXPECT_DOUBLE_EQ(vel.getVy(), vy);
        EXPECT_DOUBLE_EQ(vel.getVw(), vw);
    }
}

// ============================================
// 12. Test copy constructor với const correctness
// ============================================

TEST_F(VelocityTest, ConstObject_CanBeCopied) {
    // Arrange
    const Velocity vel1(default_vx, default_vy, default_vw);
    
    // Act
    Velocity vel2(vel1);
    
    // Assert
    EXPECT_DOUBLE_EQ(vel2.getVx(), vel1.getVx());
    EXPECT_DOUBLE_EQ(vel2.getVy(), vel1.getVy());
    EXPECT_DOUBLE_EQ(vel2.getVw(), vel1.getVw());
}

TEST_F(VelocityTest, ConstObject_GettersCanBeCalled) {
    // Arrange
    const Velocity vel(default_vx, default_vy, default_vw);
    
    // Act & Assert
    EXPECT_DOUBLE_EQ(vel.getVx(), default_vx);
    EXPECT_DOUBLE_EQ(vel.getVy(), default_vy);
    EXPECT_DOUBLE_EQ(vel.getVw(), default_vw);
}

// ============================================
// 13. Test performance (optional benchmark)
// ============================================

TEST_F(VelocityTest, DISABLED_Benchmark_CreateMillionsOfObjects) {
    // Test này bị disable mặc định, chạy khi cần benchmark
    const int NUM_OBJECTS = 1000000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        Velocity vel(static_cast<double>(i), 
                     static_cast<double>(i + 1), 
                     static_cast<double>(i + 2));
        volatile double vx = vel.getVx(); // Prevent optimization
        (void)vx;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Created " << NUM_OBJECTS << " Velocity objects in " 
              << duration.count() << " ms" << std::endl;
}