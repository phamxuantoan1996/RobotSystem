#include <system_error>
#include <stdint.h>
namespace common::ports {
    class ISocketTcpClient {
        public:
            virtual ~ISocketTcpClient() = default;

            virtual std::error_code connect() = 0;
            virtual void disconnect() = 0;
            virtual bool isConnected() const = 0;
    };
}