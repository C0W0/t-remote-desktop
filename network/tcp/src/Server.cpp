//
// Created by Terry on 2026-07-10.
//

#include "tcp/Server.h"
#include "tcp/Socket.h"

#include <string>

using namespace network;

std::expected<TcpServer, int> TcpServer::CreateServer(const uint16_t port) {
    std::expected<ListeningSocket, int> result = ListeningSocket::CreateSocket(port);
    return std::move(result).transform(
        [](ListeningSocket&& socket) { return TcpServer(std::move(socket)); }
    );
}
