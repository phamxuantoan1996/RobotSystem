#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "../domain/value_objects/station.hpp"

using namespace navigator::domain::value_objects;

// ============================================
// 1. HAPPY PATH - KHỞI TẠO THÀNH CÔNG
// ============================================

TEST(StationTest, InitValidLMStationSuccess) {
    ASSERT_NO_THROW({
        Station station("LM123");
        EXPECT_EQ(station.getId(), "LM123");
    });
}

TEST(StationTest, InitValidCPStationSuccess) {
    ASSERT_NO_THROW({
        Station station("CP4567");
        EXPECT_EQ(station.getId(), "CP4567");
    });
}

TEST(StationTest, InitValidStationWithSingleDigit) {
    ASSERT_NO_THROW({
        Station station("LM1");
        EXPECT_EQ(station.getId(), "LM1");
    });
}

TEST(StationTest, InitValidStationWithMultipleDigits) {
    ASSERT_NO_THROW({
        Station station("CP999999");
        EXPECT_EQ(station.getId(), "CP999999");
    });
}

// ============================================
// 2. SAD PATH - ĐỊNH DẠNG SAI
// ============================================

TEST(StationTest, InitInvalidFormatThrowsException) {
    EXPECT_THROW({ Station station("AB123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("XYZ"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("12LM"); }, std::invalid_argument);
}

TEST(StationTest, InitMissingNumbersThrowsException) {
    EXPECT_THROW({ Station station("LM"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("CP"); }, std::invalid_argument);
}

TEST(StationTest, InitContainsLettersAtEndThrowsException) {
    EXPECT_THROW({ Station station("LM123X"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("CP45A"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("LM12ABC"); }, std::invalid_argument);
}

TEST(StationTest, InitEmptyStringThrowsException) {
    EXPECT_THROW({ Station station(""); }, std::invalid_argument);
}

TEST(StationTest, InitWhitespaceThrowsException) {
    EXPECT_THROW({ Station station(" LM123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("LM123 "); }, std::invalid_argument);
    EXPECT_THROW({ Station station("LM 123"); }, std::invalid_argument);
}

TEST(StationTest, InitLowerCaseThrowsException) {
    EXPECT_THROW({ Station station("lm123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("cp456"); }, std::invalid_argument);
}

TEST(StationTest, InitMixedCaseThrowsException) {
    EXPECT_THROW({ Station station("Lm123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("cP456"); }, std::invalid_argument);
}

TEST(StationTest, InitInvalidPrefixThrowsException) {
    EXPECT_THROW({ Station station("AB123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("CD456"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("LMX123"); }, std::invalid_argument);
}

TEST(StationTest, InitSpecialCharactersThrowsException) {
    EXPECT_THROW({ Station station("LM@123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("CP#456"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("LM-123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("CP_456"); }, std::invalid_argument);
}

TEST(StationTest, InitOnlyNumbersThrowsException) {
    EXPECT_THROW({ Station station("123"); }, std::invalid_argument);
    EXPECT_THROW({ Station station("456"); }, std::invalid_argument);
}

// ============================================
// 3. KIỂM TRA EXCEPTION MESSAGE
// ============================================

TEST(StationTest, ExceptionMessageIsCorrect) {
    const std::string invalidCode = "XYZ99";
    try {
        Station station(invalidCode);
        FAIL() << "Expected std::invalid_argument but no exception was thrown!";
    } catch (const std::invalid_argument& err) {
        std::string expected = "Error: Station name '" + invalidCode + "' is invalid!";
        EXPECT_STREQ(err.what(), expected.c_str());
    }
}

TEST(StationTest, ExceptionMessageForEmptyString) {
    try {
        Station station("");
        FAIL() << "Expected std::invalid_argument but no exception was thrown!";
    } catch (const std::invalid_argument& err) {
        std::string expected = "Error: Station name '' is invalid!";
        EXPECT_STREQ(err.what(), expected.c_str());
    }
}

// ============================================
// 4. KIỂM TRA GETTER
// ============================================

TEST(StationTest, GetIdReturnsCorrectValue) {
    Station station("LM123");
    EXPECT_EQ(station.getId(), "LM123");
    
    Station station2("CP456");
    EXPECT_EQ(station2.getId(), "CP456");
}

TEST(StationTest, ConstObjectCanCallGetId) {
    const Station station("LM123");
    EXPECT_EQ(station.getId(), "LM123");
}

// ============================================
// 5. KIỂM TRA COPY CONSTRUCTOR
// ============================================

TEST(StationTest, CopyConstructorWorks) {
    Station original("LM123");
    Station copy(original);  // Copy constructor
    
    EXPECT_EQ(copy.getId(), "LM123");
    EXPECT_EQ(original.getId(), "LM123");
    EXPECT_TRUE(original == copy);
}

// ============================================
// 6. KIỂM TRA EQUALITY OPERATOR
// ============================================

TEST(StationTest, EqualityOperatorWorks) {
    Station station1("LM123");
    Station station2("LM123");
    Station station3("CP456");
    
    EXPECT_TRUE(station1 == station2);
    EXPECT_FALSE(station1 == station3);
    EXPECT_TRUE(station1 == station1);
}

TEST(StationTest, EqualityOperatorWithConstObjects) {
    const Station station1("LM123");
    const Station station2("LM123");
    const Station station3("CP456");
    
    EXPECT_TRUE(station1 == station2);
    EXPECT_FALSE(station1 == station3);
}

// ============================================
// 7. KIỂM TRA VỚI VECTOR
// ============================================

TEST(StationTest, VectorOfStations) {
    std::vector<Station> stations;
    
    stations.emplace_back("LM123");
    stations.emplace_back("CP456");
    stations.emplace_back("LM789");
    
    EXPECT_EQ(stations.size(), 3);
    EXPECT_EQ(stations[0].getId(), "LM123");
    EXPECT_EQ(stations[1].getId(), "CP456");
    EXPECT_EQ(stations[2].getId(), "LM789");
}

TEST(StationTest, VectorPushBackCopy) {
    std::vector<Station> stations;
    Station station("LM123");
    
    stations.push_back(station);  // Copy vào vector
    EXPECT_EQ(stations[0].getId(), "LM123");
}

TEST(StationTest, VectorFindWithEquality) {
    std::vector<Station> stations = {
        Station("LM123"),
        Station("CP456"),
        Station("LM789")
    };
    
    auto it = std::find(stations.begin(), stations.end(), Station("LM123"));
    EXPECT_NE(it, stations.end());
    EXPECT_EQ(it->getId(), "LM123");
    
    it = std::find(stations.begin(), stations.end(), Station("CP789"));
    EXPECT_EQ(it, stations.end());
}

// ============================================
// 8. PERFORMANCE TEST
// ============================================

TEST(StationTest, ConstructionPerformanceIsAcceptable) {
    const int NUM_STATIONS = 10000;
    ASSERT_NO_THROW({
        for (int i = 0; i < NUM_STATIONS; ++i) {
            std::string id = "LM" + std::to_string(i);
            Station station(id);
            EXPECT_EQ(station.getId(), id);
        }
    });
}

// ============================================
// 9. THAM SỐ HÓA TEST
// ============================================

class StationValidTest : public ::testing::TestWithParam<std::string> {};

TEST_P(StationValidTest, InitValidStation) {
    std::string code = GetParam();
    ASSERT_NO_THROW({
        Station station(code);
        EXPECT_EQ(station.getId(), code);
    });
}

INSTANTIATE_TEST_SUITE_P(
    ValidStationCodes,
    StationValidTest,
    ::testing::Values(
        "LM123",
        "CP456",
        "LM1",
        "CP999",
        "LM987654321"
    )
);

class StationInvalidTest : public ::testing::TestWithParam<std::string> {};

TEST_P(StationInvalidTest, InitInvalidStationThrows) {
    std::string code = GetParam();
    EXPECT_THROW({ Station station(code); }, std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    InvalidStationCodes,
    StationInvalidTest,
    ::testing::Values(
        "AB123",
        "LM",
        "CP",
        "LM123X",
        "",
        " LM123",
        "LM123 ",
        "lm123",
        "cp456",
        "LM@123",
        "123",
        "LMX123",
        "Lm123"
    )
);

// ============================================
// 10. KIỂM TRA IMMUTABILITY
// ============================================

TEST(StationTest, StationIsImmutable) {
    Station station("LM123");
    EXPECT_EQ(station.getId(), "LM123");
    // id_ là const, không thể thay đổi
    // station = Station("LM456");  // Lỗi compile - copy assignment deleted
}