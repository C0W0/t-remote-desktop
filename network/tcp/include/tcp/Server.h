//
// Created by Terry on 2026-07-10.
//
#ifndef TRDP_SERVER_H
#define TRDP_SERVER_H

#include <cstdint>
#include <expected>
#include <vector>

#include "Socket.h"

namespace network {
    class TcpServer {
    public:
        static std::expected<TcpServer, int> CreateServer(uint16_t port);
        ~TcpServer() = default;

        TcpServer(TcpServer&& other) noexcept : serverSocket_(std::move(other.serverSocket_)) {
            connection_ = other.connection_.exchange(nullptr);
        }

        TcpServer& operator=(TcpServer&& other) noexcept {
            serverSocket_ = std::move(other.serverSocket_);
            connection_ = other.connection_.exchange(nullptr);
            return *this;
        }

        TcpServer(const TcpServer&) = delete;
        TcpServer& operator=(const TcpServer&) = delete;

        std::expected<AddrInfo, int> accept();
        // TODO: register control callbacks instead of doing these unsafe calls
        std::expected<int, int> recv(std::span<char> buffer);
        // TODO: register control callbacks instead of doing these unsafe calls
        std::expected<void, int> send(std::string_view buffer);
    private:
        explicit TcpServer(ListeningSocket&& socket) : serverSocket_{std::move(socket)} {}
    private:
        ListeningSocket serverSocket_;
        std::atomic<std::shared_ptr<ConnectionSocket>> connection_;
    };
};


#endif //TRDP_SERVER_H