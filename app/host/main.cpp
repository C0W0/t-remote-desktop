#include <iostream>
#include <string>
#include <thread>

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

    std::atomic<bool> closed{false};
    std::jthread thread;
    server.onAccept([&thread, &closed, &server](network::AddrInfo addrInfo, network::ConnectionSocket connectionSocket, network::ArcConnectionSocket& arcConnectionSocket) {
        closed = false;
        std::println(std::cout, "accepted new connection from {}:{}", addrInfo.address, addrInfo.port);

        if (!arcConnectionSocket.load()) {
            arcConnectionSocket.store(std::make_shared<network::ConnectionSocket>(std::move(connectionSocket)));
            thread = std::jthread([connection = arcConnectionSocket.load(), &closed, &arcConnectionSocket, &server] {
                std::string buffer;
                buffer.resize(DEFAULT_BUFLEN);

                do {
                    auto recvResult = connection->recv({buffer.data(), buffer.size()});
                    if (!recvResult.has_value()) {
                        if (recvResult.error() == 0) {
                            std::println(std::cout, "client closed the connection");
                        }
                        else {
                            std::println(std::cout, "failed to recv");
                        }
                        break;
                    }
                    std::println(std::cout, "received {}", recvResult.value());
                    std::println(std::cout, "content received: {}", std::string_view{buffer.data(), static_cast<unsigned long long>(recvResult.value())});

                    auto sendResult = connection->send("Response from server");
                    if (!sendResult.has_value()) {
                        std::println(std::cout, "failed to send");
                        break;
                    }
                } while (!buffer.starts_with("exit"));

                closed = true;
                arcConnectionSocket.store(nullptr);
                if (buffer.starts_with("exit")) {
                    server.abortListening();
                }
            });
        }
        else {
            connectionSocket.send("Connection busy");
        }
    });

    while (!closed) {
        auto acceptResult = server.accept();
        if (!acceptResult.has_value()) {
            if (acceptResult.error() == 10004) {
                std::println(std::cout, "server closed");
                break;
            }
            std::println(std::cout, "failed to accept new connection");
            return 1;
        }
    }

    return 0;
}
