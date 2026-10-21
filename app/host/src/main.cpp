#include "server_app/ServerApp.h"
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
    std::jthread serverThread;
    capture::FrameQueue frameQueue;
    network::ArcConnectionSocket arcConnectionSocket = nullptr;
    server.onAccept([&serverThread, &shouldContinue, &arcConnectionSocket, &frameQueue](network::AddrInfo addrInfo, network::ConnectionSocket connectionSocket, network::TcpServer& server) {
        std::println("accepted new connection from {}:{}", addrInfo.address, addrInfo.port);
        auto connectionSocketPtr = std::make_shared<network::ConnectionSocket>(std::move(connectionSocket));
        std::shared_ptr<network::ConnectionSocket> nullSocketPtr = nullptr;
        if (arcConnectionSocket.compare_exchange_strong(nullSocketPtr, connectionSocketPtr)) {
            serverThread = ServerApp::run(arcConnectionSocket, frameQueue, [&shouldContinue, &server] {
                shouldContinue.store(false);
                server.abortListening();
            });
        } else {
            auto _ = connectionSocketPtr->send(
                network::serializeServerResponse(network::ResponseStatus::ServerBusy, "")
            );
        }
    });

    std::jthread captureThread = capture::runCapturer(
        "/Users/terry/workspace/t-remote-desktop/assets/window_xp.jpg",
        shouldContinue,
        frameQueue
    );

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
