#pragma once
namespace lift::domain::value_objects {
    class LiftTarget {
        private:
            // Thuộc tính private và hằng số (const) để đảm bảo tính bất biến sau khi khởi tạo
            const int target;

            // Các quy tắc cấu hình nghiệp vụ (Business Domain Rules)
            // Bạn có thể thay đổi các giá trị min, max này tùy thuộc vào thiết kế hệ thống
            static constexpr int MIN_TARGET = 1;
            static constexpr int MAX_TARGET = 50;

        public:
            /**
            * @brief Hàm khởi tạo duy nhất cho LiftTarget.
            * @param targetValue Giá trị tầng đích muốn gán.
            * @throws std::invalid_argument Nếu giá trị vượt quá khoảng giới hạn cho phép (min, max).
            */
            explicit LiftTarget(int targetValue);

            // Hàm huỷ mặc định
            ~LiftTarget() = default;

            /**
            * @brief Hàm lấy giá trị target (Getter).
            * Được đánh dấu `const` để không làm thay đổi trạng thái của đối tượng.
            */
            int getTarget() const;

            /**
            * @brief Định nghĩa toán tử so sánh bằng (Value Equality).
            * Một trong những đặc trưng cốt lõi của Value Object: Hai đối tượng bằng nhau nếu giá trị của chúng bằng nhau.
            */
            bool operator==(const LiftTarget& other) const;

            bool operator!=(const LiftTarget& other) const;
    };
}