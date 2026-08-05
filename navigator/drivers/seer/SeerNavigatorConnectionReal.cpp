#include "SeerNavigatorConnectionReal.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <system_error>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <vector>

namespace navigator::drivers::seer {
    SeerNavigatorConnectionReal::SeerNavigatorConnectionReal(const std::string& host,uint16_t port, uint32_t timeout) 
    : socketFd_(1),host_(host),port_(port),timeout_(timeout)
    {}

    SeerNavigatorConnectionReal::~SeerNavigatorConnectionReal()
    {
        disconnect();
    }

    std::error_code SeerNavigatorConnectionReal::connect()
    {
        std::lock_guard<std::mutex> lk(connectedMutex_);
        socketFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if(socketFd_ < 0)
        {
            return std::make_error_code(std::errc::network_unreachable);
        }

        // TCP options
        int on = 1;
        ::setsockopt(socketFd_, SOL_SOCKET,  SO_KEEPALIVE,  &on, sizeof(on));
        ::setsockopt(socketFd_, IPPROTO_TCP, TCP_KEEPIDLE,  &on, sizeof(on));
        ::setsockopt(socketFd_, IPPROTO_TCP, TCP_KEEPINTVL, &on, sizeof(on));
        ::setsockopt(socketFd_, IPPROTO_TCP, TCP_NODELAY,   &on, sizeof(on));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port_);

        if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
            ::close(socketFd_); 
            socketFd_ = -1;
            return std::make_error_code(std::errc::invalid_argument);
        }

        // ── Set non-blocking trước khi connect ───────────────────────────────────
        int flags = ::fcntl(socketFd_, F_GETFL, 0);
        ::fcntl(socketFd_, F_SETFL, flags | O_NONBLOCK);

        // ── connect() — trả về ngay với EINPROGRESS ───────────────────────────────
        int ret = ::connect(socketFd_,
                            reinterpret_cast<sockaddr*>(&addr),
                            sizeof(addr));

        if (ret < 0 && errno != EINPROGRESS) 
        {
            // Lỗi thật sự — không phải đang kết nối
            ::close(socketFd_); 
            socketFd_ = -1;
            return std::make_error_code(std::errc::connection_refused);
        }

        // ── Dùng select() chờ tối đa connectTimeoutMs ────────────────────────────
        // const int connectTimeoutMs = 3000;  // 3 giây

        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(socketFd_, &wfds);

        struct timeval tv {
            timeout_ / 1000,
            (timeout_ % 1000) * 1000
        };

        ret = ::select(socketFd_ + 1, nullptr, &wfds, nullptr, &tv);

        if (ret == 0) {
            // Timeout — SEER không phản hồi
            ::close(socketFd_); socketFd_ = -1;
            return std::make_error_code(std::errc::timed_out);
        }

        if (ret < 0) {
            // select() error
            ::close(socketFd_); socketFd_ = -1;
            return std::make_error_code(std::errc::connection_refused);
        }

        // ── Kiểm tra connect có thực sự thành công không ─────────────────────────
        int       err    = 0;
        socklen_t errlen = sizeof(err);
        ::getsockopt(socketFd_, SOL_SOCKET, SO_ERROR, &err, &errlen);

        if (err != 0) {
            // Connect thất bại (connection refused, network unreachable...)
            ::close(socketFd_); socketFd_ = -1;
            return std::error_code(err, std::system_category());
        }

        // ── Restore blocking mode cho send/recv bình thường ──────────────────────
        ::fcntl(socketFd_, F_SETFL, flags);  // bỏ O_NONBLOCK

        connected_ = true;


        return {};
    }

    void SeerNavigatorConnectionReal::disconnect()
    {
        std::lock_guard<std::mutex> lk(connectedMutex_);
        connected_ = false;
        if (socketFd_ >= 0) {
            ::shutdown(socketFd_, SHUT_RDWR);
            ::close(socketFd_);
            socketFd_ = -1;
        }
    }

    bool SeerNavigatorConnectionReal::isConnected() const
    {
        std::lock_guard<std::mutex> lk(connectedMutex_);
        return connected_;
    }


    // ─────────────────────────────────────────────────────────────────────────────
    // sendRequest — the entire Q&A cycle under one lock
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<SeerNavigatorFrame> SeerNavigatorConnectionReal::sendRequest(const SeerNavigatorFrame& req)
    {
        std::lock_guard<std::mutex> lk(ioMutex_);
        if (!connected_)
            return std::nullopt;

        auto encoded = SeerNavigatorFrameCodec::encode(req);
        if (!sendRaw(encoded)) {
            connected_ = false;
            return std::nullopt;
        }
        return recvFrame();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // sendRaw — loop until all bytes sent
    // ─────────────────────────────────────────────────────────────────────────────
    bool SeerNavigatorConnectionReal::sendRaw(const std::vector<uint8_t>& bytes)
    {
        size_t sent = 0;
        while (sent < bytes.size()) {
            ssize_t n = ::send(socketFd_,
                            bytes.data() + sent,
                            bytes.size() - sent,
                            MSG_NOSIGNAL);
            
            if (n > 0) {
                sent += static_cast<size_t>(n);
            } else if (n == 0) {
                return false;  // Connection closed
            } else { // n < 0
                if (errno == EINTR) {
                    continue;  // Thử lại nếu bị interrupt
                }
                return false;  // Các lỗi khác
            }
        }
        return true;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // recvFrame — read header (16 bytes) then payload (m_length bytes)
    // doc header ve truoc, sau do tinh toan kich thuoc data, roi moi doc data
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<SeerNavigatorFrame> SeerNavigatorConnectionReal::recvFrame()
    {
        // Step 1: read 16-byte header
        uint8_t headerBuf[HEADER_SIZE];
        if (!recvExact(headerBuf, HEADER_SIZE))
            return std::nullopt;
        if (!SeerNavigatorFrameCodec::isValidHeader(headerBuf)) 
        {
            // Per doc: malformed header causes robot to close connection
            connected_ = false;
            return std::nullopt;
        }
        // Step 2: read payload whose size is in header bytes [4-7]
        const uint32_t payLen = SeerNavigatorFrameCodec::payloadLength(headerBuf);
        std::vector<uint8_t> fullBuf(HEADER_SIZE + payLen);
        std::memcpy(fullBuf.data(), headerBuf, HEADER_SIZE);

        if (payLen > 0) {
            if (!recvExact(fullBuf.data() + HEADER_SIZE, payLen)) {
                connected_ = false;
                return std::nullopt;
            }
        }
        return SeerNavigatorFrameCodec::decode(fullBuf);
    }


    // ─────────────────────────────────────────────────────────────────────────────
    // recvExact — read exactly n bytes with timeout using select()
    // ─────────────────────────────────────────────────────────────────────────────
    bool SeerNavigatorConnectionReal::recvExact(uint8_t* buf, size_t n)
    {
        size_t received = 0;
        auto start_time = std::chrono::steady_clock::now();
        int remainingTimeout = timeout_;

        struct pollfd pfd;
        pfd.fd = socketFd_;
        pfd.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;

        while (received < n) 
        {
            // 1. Kiểm tra nếu đã hết thời gian chờ tổng thể
            if (remainingTimeout < 0 && timeout_ >= 0) return false;

            int ready = ::poll(&pfd, 1, remainingTimeout);

            if (ready < 0) {
                // 2. Xử lý tín hiệu ngắt (EINTR): Thử lại thay vì thoát
                if (errno == EINTR) continue;
                return false; // Các lỗi hệ thống khác
            }
            
            if (ready == 0) return false; // Timeout

            // 3. Đọc dữ liệu
            ssize_t r = ::recv(socketFd_, buf + received, n - received, 0);
            
            if (r < 0) {
                if (errno == EINTR) continue;
                return false; // Lỗi kết nối
            }
            if (r == 0) return false; // Robot đóng kết nối

            received += static_cast<size_t>(r);

            // 4. Cập nhật lại thời gian còn lại (Tránh chờ quá timeoutMs ban đầu)
            if (timeout_ > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time
                ).count();
                remainingTimeout = timeout_ - static_cast<int>(elapsed);
                if (remainingTimeout <= 0 && received < n) return false;
            }
        }
        return true;
    }
}