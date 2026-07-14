#include <iostream>
#include <string>
#include <memory>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "tcp/Server.h"

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

int main() {
    auto result = network::TcpServer::CreateServer(27015);
    if (!result.has_value()) {
        std::println(std::cout, "failed to create server");
        return 1;
    }
    network::TcpServer& server = result.value();
    auto acceptResult = server.accept();
    if (!acceptResult.has_value()) {
        std::println(std::cout, "failed to accept new connection");
        return 1;
    }
    std::println(std::cout, "accepted new connection from {}:{}", acceptResult.value().address, acceptResult.value().port);
    std::string buffer;
    buffer.resize(DEFAULT_BUFLEN);

    auto recvResult = server.recv({buffer.data(), buffer.size()});
    if (!recvResult.has_value()) {
        std::println(std::cout, "failed to recv");
        return 1;
    }
    std::println(std::cout, "received {}", recvResult.value());
    std::println(std::cout, "content received: {}", std::string_view{buffer.data(), static_cast<unsigned long long>(recvResult.value())});

    auto sendResult = server.send("Response from server");
    if (!sendResult.has_value()) {
        std::println(std::cout, "failed to send");
        return 1;
    }

    return 0;
}
