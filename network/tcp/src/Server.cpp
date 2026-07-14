//
// Created by Terry on 2026-07-10.
//

#include "tcp/Server.h"

#include <iostream>
#include <print>

#include "tcp/Socket.h"

#include <string>

using namespace network;

std::expected<TcpServer, int> TcpServer::CreateServer(const uint16_t port) {
    std::expected<ListeningSocket, int> result = ListeningSocket::Listen(port);
    return std::move(result).transform([port](ListeningSocket&& socket) {
        std::println(std::cout, "Listening on localhost:{}", port);
        return TcpServer(std::move(socket));
    });
}

std::expected<AddrInfo, int> TcpServer::accept() {
    AddrInfo addrInfo;
    std::expected<ConnectionSocket, int> result = ConnectionSocket::Accept(serverSocket_, &addrInfo);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    if (!connection_.load()) {
        ArcConnectionSocket connectionSocket = ConnectionSocket::toArcConnectionSocket(std::move(result.value()));
        connection_.store(std::move(connectionSocket));
        // TODO: Spawn thread to handle traffic
    }
    else {
        ConnectionSocket& connectionSocket = result.value();
        connectionSocket.send("Connection busy");
    }
    return addrInfo;
}

std::expected<int, int> TcpServer::recv(std::span<char> buffer) {
    const std::shared_ptr<ConnectionSocket> ptr = connection_.load();
    if (!ptr) {
        return std::unexpected(0);
    }
    return ptr->recv(buffer);
}

std::expected<void, int> TcpServer::send(std::string_view buffer) {
    const std::shared_ptr<ConnectionSocket> ptr = connection_.load();
    if (!ptr) {
        return std::unexpected(0);
    }
    return ptr->send(buffer);
}
