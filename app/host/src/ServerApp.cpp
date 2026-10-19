//
// Created by Terry Zha on 10/17/26.
//

#include "host/ServerApp.h"
#include "protocol/Transport.h"
#include "protocol/client/Control.h"
#include "protocol/server/Response.h"

#include <print>

#define DEFAULT_BUFLEN 512

namespace ServerApp {
std::jthread
run(network::ArcConnectionSocket& connectionSocket, const std::function<void()>& abortCommand) {
    using namespace network;
    
    std::jthread threadHandle([&] {
        auto connection = connectionSocket.load();
        std::string buffer;
        buffer.reserve(DEFAULT_BUFLEN);
        
        bool exit = false;
        while (!exit) {
            buffer.resize(TransportHeader::networkSize());
            auto recvResult = connection->recv(
                {buffer.data(), buffer.size()},
                TransportHeader::networkSize()
            );
            if (!recvResult.has_value()) {
                if (recvResult.error() == 0) {
                    std::println("client closed the connection");
                }
                else {
                    std::println("failed to recv");
                }
                break;
            }

            auto deserialized = deserializeClientHeader(
                {buffer.data(), buffer.size()}
            );
            if (!deserialized) {
                std::println("failed to deserialize Header: {}", deserialized.error());
                break;
            }

            TransportHeader header = std::move(deserialized).value();
            buffer.clear();
            buffer.resize(header.length);
            recvResult = connection->recv({buffer.data(), buffer.size()}, header.length);
            if (!recvResult.has_value()) {
                if (recvResult.error() == 0) {
                    std::println("client closed the connection");
                }
                else {
                    std::println("failed to recv");
                }
                break;
            }

            switch (header.getMessageTypeClient()) {
                case ClientMessageType::Auth: {
                    std::println("authentication");
                    // TODO: authentication
                    std::string respMsg = "Authentication successful";
                    std::vector<char> respSerialized = serializeServerResponse(ResponseStatus::Ok, respMsg);
                    auto sendResult = connection->send({respSerialized.data(), respSerialized.size()});
                    if (!sendResult.has_value()) {
                        std::println("failed to send");
                        exit = true;
                    }
                    break;
                }
                case ClientMessageType::Message: {
                    std::println("message");
                    std::println("content: {}", buffer);
                    if (buffer.starts_with("exit")) {
                        exit = true;
                    }
                    break;
                }
                case ClientMessageType::Disconnect: {
                    std::println("disconnect");
                    exit = true;
                    break;
                }
                case ClientMessageType::Control: {
                    auto deserializedMeta = deserializeControlMeta(buffer);
                    if (!deserializedMeta.has_value()) {
                        std::println("failed to deserialize ControlMeta: {}", deserializedMeta.error());
                        break;
                    }
                    ControlMeta meta = std::move(deserializedMeta).value();
                    std::println("the server received a control packet: ", toString(meta.type));
                    break;
                }
            }
        }

        connectionSocket.store(nullptr);
        if (buffer.starts_with("exit")) {
            abortCommand();
        }
    });
    
    return threadHandle;
}
}
