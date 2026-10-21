#include "protocol/server/Image.h"
#include "protocol/shared/Message.h"
#include "protocol/Transport.h"
#include "tcp/Socket.h"

#include <print>

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
    auto sendResult = socket.send(network::serializeMessage("image"));
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
        auto frameHeaderDeserialized = network::deserializeFrameHeader(buffer);
        if (!frameHeaderDeserialized) {
            std::println("failed to deserialize FrameHeader: {}", frameHeaderDeserialized.error());
            return 1;
        }
        network::FrameHeader frameHeader = std::move(frameHeaderDeserialized).value();
        std::println(
            "Received FrameHeader:\n\tVersion: {}\n\tImage Type: {}\n\tSeq: {}\n\tImageSize: {}",
            frameHeader.version, network::toString(frameHeader.type), frameHeader.seq, frameHeader.size
        );
        std::println("{} bytes of binary data received after the image header", buffer.length()- network::FrameHeader::networkSize());
    }

    // std::println("content received: {}", std::string_view{buffer.data(), header.length});

    return 0;
}