//
// Created by Terry on 2026-07-10.
//
#ifndef TRDP_SERVER_H
#define TRDP_SERVER_H

#include <cstdint>
#include <expected>
#include "ServerSocket.h"

namespace network {
    class TcpServer {
    public:
        static std::expected<TcpServer, int> CreateListeningSocket(uint16_t port);

        TcpServer(TcpServer&& server) = default;
        TcpServer& operator=(TcpServer&& server) = default;
        ~TcpServer() = default;

        TcpServer(const TcpServer&) = delete;
        TcpServer& operator=(const TcpServer&) = delete;
    private:
        explicit TcpServer(ServerSocket&& socket) : serverSocket_{std::move(socket)} {}
    private:
        ServerSocket serverSocket_;
    };
};


#endif //TRDP_SERVER_H