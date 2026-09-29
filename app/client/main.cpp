#include <print>

#include "tcp/Socket.h"
#include "protocol/Transport.h"

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

int main() {
    auto result = network::ConnectionSocket::Connect("localhost", 27015);
    if (!result) {
        std::println("Failed to connect to the server");
        return 1;
    }
    network::ConnectionSocket& socket = result.value();

    auto sendResult = socket.send(network::serializeHeaderA(network::TransportHeader {
        network::MessageType::Auth,
        0,
        0
    }));
    if (!sendResult.has_value()) {
        std::println("failed to send");
        return 1;
    }

    std::string buffer;
    buffer.resize(DEFAULT_BUFLEN);

    auto recvResult = socket.recv({buffer.data(), buffer.size()});
    if (!recvResult.has_value()) {
        std::println("failed to recv: {}", recvResult.error());
        return 1;
    }
    std::println("received {}", recvResult.value());
    std::println("content received: {}", std::string_view{buffer.data(), static_cast<unsigned long long>(recvResult.value())});

    return 0;
}