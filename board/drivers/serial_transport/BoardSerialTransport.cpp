#include "BoardSerialTransport.hpp"
#include <algorithm>
#include <fcntl.h>
#include <thread>

namespace board::drivers::serial_transport {
    speed_t BoardSerialTransport::getBaudRateConstant() 
    {
        switch(config_.baudrate) {
            case 9600: return B9600;
            case 19200: return B19200;
            case 38400: return B38400;
            case 57600: return B57600;
            case 115200: return B115200;
            default: return B9600;
        }
    }

    std::error_code BoardSerialTransport::configurePort() {
        struct termios tty;
        
        if (tcgetattr(serialFd_, &tty) != 0) {
            return std::error_code(errno, std::system_category());
        }
        
        // Set baud rate
        speed_t baud = getBaudRateConstant();
        cfsetospeed(&tty, baud);
        cfsetispeed(&tty, baud);
        
        // Configure settings
        tty.c_cflag &= ~PARENB;        // No parity
        tty.c_cflag &= ~CSTOPB;        // 1 stop bit
        tty.c_cflag &= ~CSIZE;          // Clear data size bits
        tty.c_cflag |= CS8;             // 8 data bits
        tty.c_cflag |= CREAD | CLOCAL;  // Turn on READ, ignore modem controls
        
        // Disable flow control
        tty.c_cflag &= ~CRTSCTS;
        
        // Configure non-canonical mode
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        
        // Configure input mode
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(INLCR | ICRNL | IGNCR);
        
        // Configure output mode
        tty.c_oflag &= ~OPOST;
        
        // Set timeout for read
        tty.c_cc[VMIN] = 0;   // Don't wait for minimum bytes
        tty.c_cc[VTIME] = std::max(1, static_cast<int>(config_.timeout / 100));  // Timeout in deciseconds
        
        
        // Apply configuration
        if (tcsetattr(serialFd_, TCSANOW, &tty) != 0) {
            return std::error_code(errno, std::system_category());
        }
        
        return std::error_code(); // Success
    }

    std::error_code BoardSerialTransport::connect() {
        std::lock_guard<std::mutex> lock(portMutex_);
        
        // Disconnect if already connected
        if (serialFd_ != -1) 
        {
            ::close(serialFd_);
            serialFd_ = -1;
            connected_ = false;
        }
        
        // Open serial port
        serialFd_ = ::open(config_.serial_port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        
        if (serialFd_ == -1) 
        {
            connected_ = false;
            return std::error_code(errno, std::system_category());
        }
        
        // Set to blocking mode for operations
        int flags = fcntl(serialFd_, F_GETFL, 0);
        if (flags == -1) {
            int err = errno;
            ::close(serialFd_);
            serialFd_ = -1;
            connected_ = false;
            return std::error_code(err, std::system_category());
        }
        
        if (fcntl(serialFd_, F_SETFL, flags & ~O_NONBLOCK) == -1) {
            int err = errno;
            ::close(serialFd_);
            serialFd_ = -1;
            connected_ = false;
            return std::error_code(err, std::system_category());
        }
        
        // Configure port settings
        auto ec = configurePort();
        if (ec) {
            ::close(serialFd_);
            serialFd_ = -1;
            connected_ = false;
            return ec;
        }
        
        connected_ = true;
        return std::error_code();
    }

    std::error_code BoardSerialTransport::reconnect(int delay_ms) 
    {
        std::cout << "Attempting to reconnect to " << config_.serial_port << "..." << std::endl;
        
        // Disconnect first
        disconnect();
        
        // Wait before reconnecting (optional)
        if (delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        
        // Try to connect
        auto ec = connect();
        
        if (ec) {
            std::cerr << "Reconnection failed: " << ec.message() << std::endl;
        } else {
            std::cout << "Reconnected successfully!" << std::endl;
        }
        
        return ec;
    }

    void BoardSerialTransport::disconnect() 
    {
        std::lock_guard<std::mutex> lock(portMutex_);
        
        if (serialFd_ != -1) {
            ::close(serialFd_);
            serialFd_ = -1;
            connected_ = false;
        }
    }

    bool BoardSerialTransport::isConnected() const 
    {
        std::lock_guard<std::mutex> lock(portMutex_);
        return connected_;
    }

    std::error_code BoardSerialTransport::readExactly(std::string& data, size_t n, int timeout_ms)
    {
        data.clear();
        data.reserve(n);

        int local_fd;
        {
            std::lock_guard<std::mutex> lock(portMutex_);
            if (serialFd_ < 0)
                return std::make_error_code(std::errc::not_connected);
            local_fd = serialFd_;
        }

        size_t total_read = 0;
        auto start = std::chrono::steady_clock::now();

        while (total_read < n)
        {
            // timeout tổng
            auto now = std::chrono::steady_clock::now();
            int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed > timeout_ms)
            {
                return std::make_error_code(std::errc::timed_out);
            }

            // chờ dữ liệu
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(local_fd, &readfds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100 * 1000; // 100ms chunk

            int ret = select(local_fd + 1, &readfds, nullptr, nullptr, &tv);

            if (ret < 0)
            {
                if (errno == EINTR)
                    continue;

                return std::error_code(errno, std::generic_category());
            }

            if (ret == 0)
            {
                continue; // chưa có data → loop tiếp
            }

            if (FD_ISSET(local_fd, &readfds))
            {
                char buffer[256];

                size_t remaining = n - total_read;
                size_t to_read = std::min(remaining, sizeof(buffer));

                ssize_t bytes_read = ::read(local_fd, buffer, to_read);

                if (bytes_read < 0)
                {
                    if (errno == EINTR)
                        continue;

                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;

                    return std::error_code(errno, std::generic_category());
                }

                if (bytes_read == 0)
                {
                    // device disconnected / EOF
                    return std::make_error_code(std::errc::io_error);
                }

                data.append(buffer, buffer + bytes_read);
                total_read += static_cast<size_t>(bytes_read);
            }
        }

        return {}; // success
    }

    std::error_code BoardSerialTransport::readUntil(std::string& data, char delimiter, int timeout_ms)
    {
        data.clear();

        int local_fd;
        {
            std::lock_guard<std::mutex> lock(portMutex_);
            if (serialFd_ < 0)
                return std::make_error_code(std::errc::not_connected);
            local_fd = serialFd_;

            
            tcflush(local_fd, TCIFLUSH);

        }

        auto start = std::chrono::steady_clock::now();

        while (true)
        {
            // check timeout tổng
            auto now = std::chrono::steady_clock::now();
            int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed > timeout_ms)
            {
                return std::make_error_code(std::errc::timed_out);
            }

            // 📡 chờ dữ liệu bằng select
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(local_fd, &readfds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100 * 1000; // 100ms polling

            int ret = select(local_fd + 1, &readfds, nullptr, nullptr, &tv);

            if (ret < 0)
            {
                if (errno == EINTR)
                    continue;
                return std::error_code(errno, std::generic_category());
            }

            if (ret == 0)
            {
                continue; // chưa có data → loop tiếp
            }

            if (FD_ISSET(local_fd, &readfds))
            {
                char ch;
                ssize_t n = ::read(local_fd, &ch, 1);

                if (n < 0)
                {
                    if (errno == EINTR)
                        continue;

                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;

                    return std::error_code(errno, std::generic_category());
                }

                if (n == 0)
                {
                    // EOF / device disconnected
                    return std::make_error_code(std::errc::io_error);
                }

                if (ch != delimiter)
                {
                    data.push_back(ch);
                    continue;   
                }
                return {}; // success
            }
        }
    }

    std::error_code BoardSerialTransport::write(const std::string& data, int timeout_ms)
    {
        if (data.empty())
            return {};

        int local_fd;
        {
            std::lock_guard<std::mutex> lock(portMutex_);
            if (serialFd_ < 0)
                return std::make_error_code(std::errc::not_connected);
            local_fd = serialFd_;
        }

        size_t total_written = 0;
        auto start = std::chrono::steady_clock::now();

        while (total_written < data.size())
        {
            // ⏱ check timeout
            auto now = std::chrono::steady_clock::now();
            int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed > timeout_ms)
            {
                return std::make_error_code(std::errc::timed_out);
            }

            // 📡 dùng select để chờ writable
            fd_set writefds;
            FD_ZERO(&writefds);
            FD_SET(local_fd, &writefds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100 * 1000; // 100ms chunk

            int ret = select(local_fd + 1, nullptr, &writefds, nullptr, &tv);

            if (ret < 0)
            {
                if (errno == EINTR)
                    continue;
                return std::error_code(errno, std::generic_category());
            }

            if (ret == 0)
            {
                continue; // timeout nhỏ → loop tiếp
            }

            if (FD_ISSET(local_fd, &writefds))
            {
                ssize_t n = ::write(
                    local_fd,
                    data.data() + total_written,
                    data.size() - total_written
                );

                if (n < 0)
                {
                    if (errno == EINTR)
                        continue;

                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;

                    return std::error_code(errno, std::generic_category());
                }

                total_written += static_cast<size_t>(n);
            }
        }

        return {}; // success
    }
}