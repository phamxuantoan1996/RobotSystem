#include "../board/drivers/tcp_transport/BoardTcpTransport.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <mutex>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

namespace board::drivers::tcp_transport {

    std::error_code BoardTcpTransport::connect()
    {
        // 1. Đảm bảo reset cờ trạng thái trước khi thực hiện kết nối mới
        {
            std::lock_guard<std::mutex> lk(portMutex_);
            connected_ = false;
            if (socketFd_ >= 0) {
                ::close(socketFd_);
                socketFd_ = -1;
            }
        }

        int local_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (local_fd < 0) {
            return std::make_error_code(std::errc::network_unreachable);
        }

        // 2. TCP Options (Sửa lại cấu hình KeepAlive hợp lý cho Arduino)
        int keepalive_on = 1;
        int keepidle_time = 10;  // 10 giây im lặng bắt đầu gửi probe (Thay vì 1s)
        int keepinterval = 3;    // Gửi lại sau mỗi 3 giây nếu không nhận phản hồi probe
        int nodelay_on = 1;

        ::setsockopt(local_fd, SOL_SOCKET,  SO_KEEPALIVE,  &keepalive_on, sizeof(keepalive_on));
        ::setsockopt(local_fd, IPPROTO_TCP, TCP_KEEPIDLE,  &keepidle_time, sizeof(keepidle_time));
        ::setsockopt(local_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepinterval, sizeof(keepinterval));
        ::setsockopt(local_fd, IPPROTO_TCP, TCP_NODELAY,   &nodelay_on, sizeof(nodelay_on));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(config_.port);

        if (::inet_pton(AF_INET, config_.host.c_str(), &addr.sin_addr) <= 0) {
            ::close(local_fd);
            return std::make_error_code(std::errc::invalid_argument);
        }

        // 3. Chuyển sang Non-blocking mode
        int flags = ::fcntl(local_fd, F_GETFL, 0);
        ::fcntl(local_fd, F_SETFL, flags | O_NONBLOCK);

        // Yêu cầu kết nối
        int ret = ::connect(local_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        // if (ret < 0 && errno != EINPROGRESS) {
        //     ::close(local_fd);
        //     return std::error_code(errno, std::system_category());
        // }
        if (ret < 0) {
            if (errno != EINPROGRESS) {
                // Lỗi kết nối thực sự (Sai IP, Server đóng cửa...)
                ::close(socketFd_); socketFd_ = -1;
                return std::make_error_code(std::errc::connection_refused);
            }
            // Nếu errno == EINPROGRESS -> HOÀN TOÀN BÌNH THƯỜNG, chạy tiếp xuống select() bên dưới để đợi.
        } else {
            // Kết nối thành công ngay lập tức (Ít gặp ở chế độ Non-blocking nhưng vẫn có thể xảy ra)
            connected_ = true;
            ::fcntl(socketFd_, F_SETFL, flags); // Restore blocking
            return {};
        }

        // 4. Chờ kết nối bằng select() - KHÔNG giữ Mutex đoạn này để tránh deadlock hệ thống
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(local_fd, &wfds);

        struct timeval tv {
            config_.timeout / 1000,
            (config_.timeout % 1000) * 1000
        };

        ret = ::select(local_fd + 1, nullptr, &wfds, nullptr, &tv);

        if (ret == 0) { // Hết thời gian chờ (Timeout)
            ::close(local_fd);
            return std::make_error_code(std::errc::timed_out);
        }
        if (ret < 0) {  // Lỗi hệ thống select
            ::close(local_fd);
            return std::error_code(errno, std::system_category());
        }

        // 5. Kiểm tra trạng thái Socket thật sự sau khi select báo hiệu ghi được
        int err = 0;
        socklen_t errlen = sizeof(err);
        ::getsockopt(local_fd, SOL_SOCKET, SO_ERROR, &err, &errlen);

        if (err != 0) {
            ::close(local_fd);
            return std::error_code(err, std::system_category()); // Trả đúng mã lỗi hệ thống (ví dụ: ECONNREFUSED)
        }

        // 6. Trả lại trạng thái Blocking thông thường cho send/recv ổn định
        ::fcntl(local_fd, F_SETFL, flags); 

        // 7. Cập nhật kết quả vào biến Class một cách an toàn
        {
            std::lock_guard<std::mutex> lk(portMutex_);
            socketFd_ = local_fd;
            connected_ = true;
        }

        return {};
    }

    std::error_code BoardTcpTransport::reconnect(int delay_ms)
    {
        
    }
    bool BoardTcpTransport::isConnected() const
    {
        std::lock_guard<std::mutex> lk(portMutex_);
        return connected_;
    }
    void BoardTcpTransport::disconnect()
    {
        std::lock_guard<std::mutex> lk(portMutex_);
        connected_ = false;
        if (socketFd_ >= 0) {
            ::shutdown(socketFd_, SHUT_RDWR);
            ::close(socketFd_);
            socketFd_ = -1;
        }
    }

    std::error_code BoardTcpTransport::readExactly(std::string& data, size_t n, int timeout_ms)
    {
        return {};
    }
    std::error_code BoardTcpTransport::readUntil(std::string& data, char delimiter, int timeout_ms)
    {
        data.clear();
        int local_fd = -1;

        // Latch/Lấy fd ra một cách an toàn, giải phóng khóa ngay sau đó
        {
            std::lock_guard<std::mutex> lock(portMutex_); // Dùng mutex riêng cho việc đọc
            if (socketFd_ < 0) {
                return std::make_error_code(std::errc::not_connected);
            }
            local_fd = socketFd_;
        }

        auto start = std::chrono::steady_clock::now();
        char rx_buf[256]; // Tăng buffer lên để đọc nhanh hơn

        while (true)
        {
            auto now = std::chrono::steady_clock::now();
            int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            int remaining = timeout_ms - elapsed;

            if (remaining <= 0) return std::make_error_code(std::errc::timed_out);

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(local_fd, &readfds);

            struct timeval tv;
            tv.tv_sec = remaining / 1000;
            tv.tv_usec = (remaining % 1000) * 1000;

            // Đoạn này select() sẽ chặn (block) luồng, nhưng vì KHÔNG giữ mutex 
            // nên các luồng khác vẫn gọi hàm write() để gửi dữ liệu bình thường!
            int ret = ::select(local_fd + 1, &readfds, nullptr, nullptr, &tv);

            if (ret < 0) {
                if (errno == EINTR) continue;
                return std::error_code(errno, std::generic_category());
            }
            if (ret == 0) return std::make_error_code(std::errc::timed_out);

            if (FD_ISSET(local_fd, &readfds))
            {
                ssize_t n = ::recv(local_fd, rx_buf, sizeof(rx_buf), MSG_NOSIGNAL);
                if (n < 0) {
                    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    return std::error_code(errno, std::generic_category());
                }
                if (n == 0) return std::make_error_code(std::errc::connection_aborted);

                for (ssize_t i = 0; i < n; ++i) {
                    if (rx_buf[i] == delimiter) {
                        data.append(rx_buf, i);
                        return {};
                    }
                }
                data.append(rx_buf, n);
            }
        }
    }
    std::error_code BoardTcpTransport::write(const std::string& data, int timeout_ms)
    {
        int local_fd = -1;
    
        // Chỉ khóa để kiểm tra trạng thái kết nối
        {
            std::lock_guard<std::mutex> lock(portMutex_); // Dùng mutex riêng cho ghi
            if (socketFd_ < 0) {
                return std::make_error_code(std::errc::not_connected);
            }
            local_fd = socketFd_;
        }

        size_t sent = 0;
        while (sent < data.size()) {
            // Gửi dữ liệu không giữ mutex, giúp read và write chạy song song hoàn toàn
            ssize_t n = ::send(local_fd,
                            data.data() + sent,
                            data.size() - sent,
                            MSG_NOSIGNAL);
            
            if (n > 0) {
                sent += static_cast<size_t>(n);
            } else if (n == 0) {
                return std::make_error_code(std::errc::connection_aborted);
            } else {
                if (errno == EINTR) continue;
                return std::error_code(errno, std::generic_category());
            }
        }
        return {};
    }

}