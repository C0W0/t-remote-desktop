#include "protocol/Transport.h"
#include "tcp/Socket.h"

#include <print>

#include "protocol/shared/Message.h"

#define DEFAULT_PORT 27015
#define DEFAULT_BUFLEN 512

int main() {
    auto result = network::ConnectionSocket::Connect("localhost", DEFAULT_PORT);
    if (!result) {
        std::println("Failed to connect to the server");
        return 1;
    }
    network::ConnectionSocket& socket = result.value();

    // auto sendResult = socket.send(network::serializeHeaderA(network::TransportHeader {
    //     network::ClientMessageType::Auth,
    //     0,
    //     0
    // }));
    auto sendResult = socket.send(network::serializeMessage("test"));
    if (!sendResult.has_value()) {
        std::println("failed to send");
        return 1;
    }

    std::string buffer;
    buffer.resize(DEFAULT_BUFLEN);

    {
        auto recvResult = socket.recv({buffer.data(), buffer.size()}, network::TransportHeader::networkSize());
        if (!recvResult.has_value()) {
            std::println("failed to recv: {}", recvResult.error());
            return 1;
        }
    }

    auto deserialized = network::deserializeServerHeader(
        {buffer.data(), buffer.size()}
    );
    if (!deserialized) {
        std::println("failed to deserialize Header: {}", deserialized.error());
        return 1;
    }
    network::TransportHeader header = std::move(deserialized).value();
    buffer.clear();
    buffer.resize(header.length);

    {
        auto recvResult = socket.recv({buffer.data(), buffer.size()}, header.length);
        if (!recvResult.has_value()) {
           std::println("failed to recv: {}", recvResult.error());
           return 1;
       }
    }

    std::println("content received: {}", std::string_view{buffer.data(), header.length});

    return 0;
}