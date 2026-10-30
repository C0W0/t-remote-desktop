//
// Created by Terry Zha on 10/17/26.
//

#include "ServerApp.h"
#include "protocol/Transport.h"
#include "protocol/client/Control.h"
#include "protocol/server/Response.h"
#include "protocol/server/Image.h"
#include "common/EnumUtils.hpp"

#include <print>
#include <utility>

#define DEFAULT_BUFLEN 512

namespace ServerApp {
std::jthread run(
    network::ArcConnectionSocket& connectionSocket,
    capture::FrameChannel::Rx& rx,
    std::function<void()> abortCommand
) {
    using namespace network;
    
    std::jthread threadHandle([&connectionSocket, rx = std::move(rx), abortCommand = std::move(abortCommand)] mutable {
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
                        break;
                    }
                    if (buffer.starts_with("image")) {
                        auto imgReceived = rx.get();
                        if (!imgReceived.has_value()) {
                            std::println("failed to receive image: {}", enum_utils::toString(imgReceived.error()));
                        }
                        std::vector<char> imageBuffer = std::move(imgReceived).value();
                        std::vector<char> headerBuffers = serializeHeaderV(TransportHeader {
                            ServerMessageType::ImageFrame,
                            0,
                            static_cast<uint32_t>(FrameHeader::networkSize() + imageBuffer.size()),
                        });
                        headerBuffers.resize(TransportHeader::networkSize() + FrameHeader::networkSize());
                        serializeFrameHeaderInto(
                            { .magic = 0xFEEDF00D, .version = 0, .type = ImageType::JPG, .flags = 0, .seq = 0, .size = static_cast<uint32_t>(imageBuffer.size()) },
                            {headerBuffers.data() + TransportHeader::networkSize(), imageBuffer.size()}
                        );
                        if (!connection->send(headerBuffers).has_value()) {
                            std::println("failed to send transport or image header");
                            exit = true;
                            break;
                        }
                        if (!connection->send(imageBuffer).has_value()) {
                            std::println("failed to send image");
                            exit = true;
                            break;
                        }
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
                    std::println("the server received a control packet: ", enum_utils::toString(meta.type));
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
