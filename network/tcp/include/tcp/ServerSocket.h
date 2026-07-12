//
// Created by Terry on 2026-07-10.
//

#ifndef TRDP_SOCKET_H
#define TRDP_SOCKET_H
#include <memory>
#include <cstdint>
#include <expected>

namespace network {
    class ServerSocket {
    public:
        static std::expected<ServerSocket, int> CreateListeningSocket(uint16_t port);
    private:
        ServerSocket();
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
} // network

#endif //TRDP_SOCKET_H
