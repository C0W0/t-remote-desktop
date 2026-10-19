#include "host/ServerApp.h"
#include "protocol/server/Response.h"
#include "protocol/Transport.h"
#include "tcp/Server.h"

#include <print>
#include <string>
#include <thread>

#define DEFAULT_PORT 27015

int main() {
    auto result = network::TcpServer::CreateServer(DEFAULT_PORT);
    if (!result.has_value()) {
        std::println("failed to create server");
        return 1;
    }
    network::TcpServer& server = result.value();

    std::atomic<bool> shouldContinue{true};
    std::jthread thread;
    network::ArcConnectionSocket arcConnectionSocket = nullptr;
    server.onAccept([&thread, &shouldContinue, &arcConnectionSocket](network::AddrInfo addrInfo, network::ConnectionSocket connectionSocket, network::TcpServer& server) {
        std::println("accepted new connection from {}:{}", addrInfo.address, addrInfo.port);
        auto connectionSocketPtr = std::make_shared<network::ConnectionSocket>(std::move(connectionSocket));
        std::shared_ptr<network::ConnectionSocket> nullSocketPtr = nullptr;
        if (arcConnectionSocket.compare_exchange_strong(nullSocketPtr, std::move(connectionSocketPtr))) {
            thread = ServerApp::run(arcConnectionSocket, [&shouldContinue, &server] {
                shouldContinue.store(false);
                server.abortListening();
            });
        } else {
            network::ServerRespMeta respMeta {
                .status = network::ResponseStatus::ServerBusy,
                .messageLength = 0,
            };
            network::TransportHeader respHeader{network::ClientMessageType::Auth, 0, network::ServerRespMeta::networkSize()};

            std::vector<char> respSerialized = network::serializeHeaderV(respHeader);
            std::array<char, network::ServerRespMeta::networkSize()> respMetaSerialized = network::serializeServerRespMeta(respMeta);
            respSerialized.insert(respSerialized.end(), respMetaSerialized.begin(), respMetaSerialized.end());

            auto _ = connectionSocket.send({respSerialized.data(), respSerialized.size()});
        }
    });

    while (shouldContinue) {
        auto acceptResult = server.accept();
        if (!acceptResult.has_value()) {
            if (acceptResult.error() == network::kAcceptAborted) {
                std::println("server closed");
                break;
            }
            std::println("failed to accept new connection");
            return 1;
        }
    }

    return 0;
}
