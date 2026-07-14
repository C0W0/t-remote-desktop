#include <iostream>

#include "tcp/Socket.h"

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

int main() {
    auto result = network::ConnectionSocket::Connect("100.109.181.42", 27015);
    if (!result) {
        std::println(std::cout, "Failed to connect to the server");
        return 1;
    }
    network::ConnectionSocket& socket = result.value();

    auto sendResult = socket.send("Hello world from C++");
    if (!sendResult.has_value()) {
        std::println(std::cout, "failed to send");
        return 1;
    }

    std::string buffer;
    buffer.resize(DEFAULT_BUFLEN);

    auto recvResult = socket.recv({buffer.data(), buffer.size()});
    if (!recvResult.has_value()) {
        std::println(std::cout, "failed to recv");
        return 1;
    }
    std::println(std::cout, "received {}", recvResult.value());
    std::println(std::cout, "content received: {}", std::string_view{buffer.data(), static_cast<unsigned long long>(recvResult.value())});

    return 0;
}