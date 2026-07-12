//
// Created by Terry on 2026-07-10.
//

#include "tcp/Server.h"

#include <string>

using namespace network;

std::expected<TcpServer, int> TcpServer::CreateListeningSocket(const uint16_t port) {
    std::expected<ServerSocket, int> result = ServerSocket::CreateListeningSocket(port);
    if (result.has_value()) {
        return TcpServer(std::move(result.value()));
    }
    return std::unexpected(result.error());
}
