#include <gtest/gtest.h>
#include "location.hpp"
#include <stdexcept>

using namespace navigator::domain::value_objects;

// ============================================
// Test Fixture cho Location
// ============================================
class LocationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Khởi tạo các giá trị mặc định
        default_x = 10.5;
        default_y = 20.3;
        default_angle = 45.0;
    }

    double default_x;
    double default_y;
    double default_angle;
};

// ============================================
// 1. Test Constructor và khởi tạo
// ============================================

TEST_F(LocationTest, Constructor_WithValidValues_ShouldCreateObject) {
    // Act
    Location loc(default_x, default_y, default_angle);

    // Assert
    EXPECT_DOUBLE_EQ(loc.getX(), default_x);
    EXPECT_DOUBLE_EQ(loc.getY(), default_y);
    EXPECT_DOUBLE_EQ(loc.getAngle(), default_angle);
}

TEST_F(LocationTest, Constructor_WithZeroValues_ShouldCreateObject) {
    // Act
    Location loc(0.0, 0.0, 0.0);

    // Assert
    EXPECT_DOUBLE_EQ(loc.getX(), 0.0);
    EXPECT_DOUBLE_EQ(loc.getY(), 0.0);
    EXPECT_DOUBLE_EQ(loc.getAngle(), 0.0);
}

TEST_F(LocationTest, Constructor_WithNegativeValues_ShouldCreateObject) {
    // Act
    Location loc(-15.7, -22.3, -90.0);

    // Assert
    EXPECT_DOUBLE_EQ(loc.getX(), -15.7);
    EXPECT_DOUBLE_EQ(loc.getY(), -22.3);
    EXPECT_DOUBLE_EQ(loc.getAngle(), -90.0);
}

// ============================================
// 2. Test Validation của angle
// ============================================

TEST_F(LocationTest, Constructor_WithAngleGreaterThan180_ShouldThrowException) {
    // Assert
    EXPECT_THROW({
        Location loc(default_x, default_y, 181.0);
    }, std::invalid_argument);
}

TEST_F(LocationTest, Constructor_WithAngleEqualTo180_ShouldCreateObject) {
    // Act & Assert (không throw)
    EXPECT_NO_THROW({
        Location loc(default_x, default_y, 180.0);
    });
}

TEST_F(LocationTest, Constructor_WithAngleLessThanNegative180_ShouldThrowException) {
    // Assert
    EXPECT_THROW({
        Location loc(default_x, default_y, -181.0);
    }, std::invalid_argument);
}

TEST_F(LocationTest, Constructor_WithAngleEqualToNegative180_ShouldCreateObject) {
    // Act & Assert (không throw)
    EXPECT_NO_THROW({
        Location loc(default_x, default_y, -180.0);
    });
}

TEST_F(LocationTest, Constructor_WithAngleBetweenNegative180And180_ShouldCreateObject) {
    // Test với nhiều giá trị hợp lệ
    std::vector<double> valid_angles = {-180.0, -90.0, 0.0, 45.5, 90.0, 179.999, 180.0};
    
    for (double angle : valid_angles) {
        EXPECT_NO_THROW({
            Location loc(default_x, default_y, angle);
        }) << "Failed with angle: " << angle;
    }
}

// ============================================
// 3. Test Copy Constructor
// ============================================

TEST_F(LocationTest, CopyConstructor_ShouldCreateExactCopy) {
    // Arrange
    Location original(default_x, default_y, default_angle);

    // Act
    Location copy(original);

    // Assert
    EXPECT_DOUBLE_EQ(copy.getX(), original.getX());
    EXPECT_DOUBLE_EQ(copy.getY(), original.getY());
    EXPECT_DOUBLE_EQ(copy.getAngle(), original.getAngle());
}

TEST_F(LocationTest, CopyConstructor_WithDifferentObject_ShouldNotAffectOriginal) {
    // Arrange
    Location original(default_x, default_y, default_angle);
    
    // Act
    Location copy(original);
    // Lưu ý: copy không thể thay đổi vì các thành viên là const
    // nhưng chúng ta kiểm tra chúng vẫn giống nhau

    // Assert
    EXPECT_DOUBLE_EQ(copy.getX(), default_x);
    EXPECT_DOUBLE_EQ(copy.getY(), default_y);
    EXPECT_DOUBLE_EQ(copy.getAngle(), default_angle);
}

// // ============================================
// // 4. Test Assignment Operator
// // ============================================

// TEST_F(LocationTest, AssignmentOperator_ShouldReturnReferenceToThis) {
//     // Arrange
//     Location loc1(default_x, default_y, default_angle);
//     Location loc2(0.0, 0.0, 0.0);

//     // Act
//     Location& result = (loc2 = loc1);

//     // Assert - toán tử = trả về tham chiếu đến chính nó
//     EXPECT_EQ(&result, &loc2);
// }

// TEST_F(LocationTest, AssignmentOperator_Chaining_ShouldWorkCorrectly) {
//     // Arrange
//     Location loc1(default_x, default_y, default_angle);
//     Location loc2(0.0, 0.0, 0.0);
//     Location loc3(1.0, 2.0, 3.0);

//     // Act
//     loc3 = loc2 = loc1;

//     // Assert
//     EXPECT_DOUBLE_EQ(loc3.getX(), loc1.getX());
//     EXPECT_DOUBLE_EQ(loc3.getY(), loc1.getY());
//     EXPECT_DOUBLE_EQ(loc3.getAngle(), loc1.getAngle());
// }

// ============================================
// 5. Test Equality Operator
// ============================================

TEST_F(LocationTest, EqualityOperator_TwoIdenticalLocations_ShouldReturnTrue) {
    // Arrange
    Location loc1(default_x, default_y, default_angle);
    Location loc2(default_x, default_y, default_angle);

    // Act
    bool is_equal = (loc1 == loc2);

    // Assert
    EXPECT_TRUE(is_equal);
}

TEST_F(LocationTest, EqualityOperator_TwoDifferentX_ShouldReturnFalse) {
    // Arrange
    Location loc1(default_x, default_y, default_angle);
    Location loc2(default_x + 1.0, default_y, default_angle);

    // Act
    bool is_equal = (loc1 == loc2);

    // Assert
    EXPECT_FALSE(is_equal);
}

TEST_F(LocationTest, EqualityOperator_TwoDifferentY_ShouldReturnFalse) {
    // Arrange
    Location loc1(default_x, default_y, default_angle);
    Location loc2(default_x, default_y + 1.0, default_angle);

    // Act
    bool is_equal = (loc1 == loc2);

    // Assert
    EXPECT_FALSE(is_equal);
}

TEST_F(LocationTest, EqualityOperator_TwoDifferentAngle_ShouldReturnFalse) {
    // Arrange
    Location loc1(default_x, default_y, default_angle);
    Location loc2(default_x, default_y, default_angle + 10.0);

    // Act
    bool is_equal = (loc1 == loc2);

    // Assert
    EXPECT_FALSE(is_equal);
}

TEST_F(LocationTest, EqualityOperator_TwoLocationsWithSameValuesButDifferentPrecision_ShouldReturnFalse) {
    // Arrange
    Location loc1(0.1, 0.2, 0.3);
    Location loc2(0.1000000001, 0.2, 0.3);

    // Act
    bool is_equal = (loc1 == loc2);

    // Assert - Kiểm tra chính xác double nên phải false
    EXPECT_FALSE(is_equal);
}

TEST_F(LocationTest, EqualityOperator_CompareWithItself_ShouldReturnTrue) {
    // Arrange
    Location loc(default_x, default_y, default_angle);

    // Act
    bool is_equal = (loc == loc);

    // Assert
    EXPECT_TRUE(is_equal);
}

// ============================================
// 6. Test Getter methods
// ============================================

TEST_F(LocationTest, GetX_ShouldReturnCorrectValue) {
    // Arrange
    Location loc(default_x, default_y, default_angle);

    // Act
    double x = loc.getX();

    // Assert
    EXPECT_DOUBLE_EQ(x, default_x);
}

TEST_F(LocationTest, GetY_ShouldReturnCorrectValue) {
    // Arrange
    Location loc(default_x, default_y, default_angle);

    // Act
    double y = loc.getY();

    // Assert
    EXPECT_DOUBLE_EQ(y, default_y);
}

TEST_F(LocationTest, GetAngle_ShouldReturnCorrectValue) {
    // Arrange
    Location loc(default_x, default_y, default_angle);

    // Act
    double angle = loc.getAngle();

    // Assert
    EXPECT_DOUBLE_EQ(angle, default_angle);
}

// ============================================
// 7. Test với nhiều giá trị khác nhau
// ============================================

class LocationParameterizedTest : public ::testing::TestWithParam<std::tuple<double, double, double>> {};

TEST_P(LocationParameterizedTest, Constructor_WithVariousValues_ShouldCreateObject) {
    // Arrange
    auto [x, y, angle] = GetParam();

    // Act
    Location loc(x, y, angle);

    // Assert
    EXPECT_DOUBLE_EQ(loc.getX(), x);
    EXPECT_DOUBLE_EQ(loc.getY(), y);
    EXPECT_DOUBLE_EQ(loc.getAngle(), angle);
}

INSTANTIATE_TEST_SUITE_P(
    LocationTests,
    LocationParameterizedTest,
    ::testing::Values(
        std::make_tuple(0.0, 0.0, 0.0),
        std::make_tuple(10.5, 20.3, 45.0),
        std::make_tuple(-10.5, -20.3, -45.0),
        std::make_tuple(100.0, 200.0, 180.0),
        std::make_tuple(1.234, 5.678, 90.0),
        std::make_tuple(999.999, 888.888, -90.0)
    )
);

// ============================================
// 8. Test exception message (optional)
// ============================================

TEST_F(LocationTest, Constructor_WithInvalidAngle_ShouldThrowExceptionWithCorrectMessage) {
    // Act & Assert
    try {
        Location loc(default_x, default_y, 200.0);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "Error: Value of angle is invalid!");
    } catch (...) {
        FAIL() << "Expected std::invalid_argument";
    }
}

// ============================================
// 9. Test memory và performance (optional)
// ============================================

TEST_F(LocationTest, ObjectSize_ShouldBeReasonable) {
    // Kiểm tra kích thước object không quá lớn
    // 3 double = 24 bytes (thường)
    size_t expected_size = sizeof(double) * 3; // x_, y_, angle_
    EXPECT_LE(sizeof(Location), expected_size + 8); // Cho phép padding tối đa 8 bytes
}

// ============================================
// Main function nếu không dùng gtest_main
// ============================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}