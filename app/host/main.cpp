#include <iostream>
#include <string>
#include <thread>

#include "tcp/Server.h"
#include "protocol/Transport.h"

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
    network::ArcConnectionSocket arcConnectionSocket;
    server.onAccept([&thread, &closed, &arcConnectionSocket](network::AddrInfo addrInfo, network::ConnectionSocket connectionSocket, network::TcpServer& server) {
        closed = false;
        std::println(std::cout, "accepted new connection from {}:{}", addrInfo.address, addrInfo.port);

        if (!arcConnectionSocket.load()) {
            arcConnectionSocket.store(std::make_shared<network::ConnectionSocket>(std::move(connectionSocket)));
            thread = std::jthread([connection = arcConnectionSocket.load(), &closed, &arcConnectionSocket, &server] {
                std::string buffer;
                buffer.resize(DEFAULT_BUFLEN);
                bool exit = false;
                while (!exit) {
                    auto recvResult = connection->recv({buffer.data(), buffer.size()}, sizeof(network::TransportHeader));
                    if (!recvResult.has_value()) {
                        if (recvResult.error() == 0) {
                            std::println(std::cout, "client closed the connection");
                        }
                        else {
                            std::println(std::cout, "failed to recv");
                        }
                        break;
                    }

                    auto deserialized = network::deserializeHeader({buffer.data(), buffer.size()});
                    if (!deserialized) {
                        std::println(std::cout, "failed to deserialize Header: {}", deserialized.error());
                        break;
                    }

                    network::TransportHeader header = std::move(deserialized).value();
                    switch (header.type) {
                        case network::MessageType::Auth: {
                            std::println(std::cout, "authentication");
                            // TODO: authentication
                            std::string respMsg = "Authentication successful";
                            network::ServerRespMeta respMeta {
                                .status = network::ResponseStatus::Ok,
                                .messageLength = static_cast<uint16_t>(respMsg.length()),
                            };
                            network::TransportHeader respHeader {
                                .type = network::MessageType::Auth,
                                .flags = 0,
                                .length = static_cast<uint32_t>(sizeof(respMeta)) + respMeta.messageLength
                            };

                            std::vector<char> respSerialized = network::serializeHeaderV(respHeader);
                            std::array<char, sizeof(network::ServerRespMeta)> respMetaSerialized = network::serializeServerRespMeta(respMeta);
                            respSerialized.insert(respSerialized.end(), respMetaSerialized.begin(), respMetaSerialized.end());
                            respSerialized.insert(respSerialized.end(), respMsg.begin(), respMsg.end());

                            auto sendResult = connection->send({respSerialized.data(), respSerialized.size()});
                            if (!sendResult.has_value()) {
                                std::println(std::cout, "failed to send");
                                exit = true;
                            }
                            break;
                        }
                        case network::MessageType::Message: {
                            std::println(std::cout, "message");
                            buffer.clear();
                            buffer.resize(header.length);

                            recvResult = connection->recv({buffer.data(), buffer.size()}, header.length);
                            if (!recvResult.has_value()) {
                                if (recvResult.error() == 0) {
                                    std::println(std::cout, "client closed the connection");
                                }
                                else {
                                    std::println(std::cout, "failed to recv");
                                }
                                exit = true;
                                break;
                            }

                            std::println(std::cout, "content: {}", buffer);
                            break;
                        }
                        case network::MessageType::Disconnect: {
                            std::println(std::cout, "disconnect");
                            exit = true;
                            break;
                        }
                        case network::MessageType::ServerResp: {
                            std::println(std::cout, "server response - should not be received by the server.");
                            break;
                        }
                    }
                }

                closed = true;
                arcConnectionSocket.store(nullptr);
                if (buffer.starts_with("exit")) {
                    server.abortListening();
                }
            });
        }
        else {
            network::ServerRespMeta respMeta {
                .status = network::ResponseStatus::ServerBusy,
                .messageLength = 0,
            };
            network::TransportHeader respHeader {
                .type = network::MessageType::Auth,
                .flags = 0,
                .length = static_cast<uint32_t>(sizeof(respMeta))
            };

            std::vector<char> respSerialized = network::serializeHeaderV(respHeader);
            std::array<char, sizeof(network::ServerRespMeta)> respMetaSerialized = network::serializeServerRespMeta(respMeta);
            respSerialized.insert(respSerialized.end(), respMetaSerialized.begin(), respMetaSerialized.end());

            connectionSocket.send({respSerialized.data(), respSerialized.size()});
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
