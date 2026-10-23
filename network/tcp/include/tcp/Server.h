//
// Created by Terry on 2026-07-10.
//
#ifndef TRDP_SERVER_H
#define TRDP_SERVER_H

#include <cstdint>
#include <expected>
#include <functional>

#include "Socket.h"

namespace network {
class TcpServer {
public:
    using ConnectionHandler = std::function<void(AddrInfo, ConnectionSocket, TcpServer&)>;

    static std::expected<TcpServer, int> CreateServer(uint16_t port);

    ~TcpServer() = default;

    TcpServer(TcpServer&& other) noexcept : serverSocket_(std::move(other.serverSocket_)) {}

    TcpServer& operator=(TcpServer&& other) noexcept {
        serverSocket_ = std::move(other.serverSocket_);
        return *this;
    }

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    std::expected<AddrInfo, int> accept();

    void onAccept(ConnectionHandler&& onAccept);
    void abortListening() {
        serverSocket_.close();
    }

private:
    explicit TcpServer(ListeningSocket&& socket) : serverSocket_{std::move(socket)} {
    }

private:
    ListeningSocket serverSocket_;
    ConnectionHandler onAccept_;
};
};


#endif //TRDP_SERVER_H
