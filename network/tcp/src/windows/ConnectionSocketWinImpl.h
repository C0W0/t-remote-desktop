//
// Created by Terry on 2026-07-13.
//

#ifndef TRDP_CONNECTIONSOCKETWINIMPL_H
#define TRDP_CONNECTIONSOCKETWINIMPL_H

#include <span>

#include "tcp/Socket.h"
#include <winsock2.h>
#include <ws2tcpip.h>

namespace network {
class ConnectionSocket::Impl {
public:
    static std::expected<std::unique_ptr<Impl>, int> Accept(const ListeningSocket& listeningSocket, AddrInfo* outAddrInfo);

    // DO NOT USE THE CONSTRUCTOR. Call `Listen` to create a socket instead.
    explicit Impl() = default;
    ~Impl();

    std::expected<int, int> recv(std::span<char> buffer);
    std::expected<void, int> send(std::string_view buffer);

    void close();

private:
    SOCKET socket_ = INVALID_SOCKET;
    bool closed_ = false;
};
}

#endif //TRDP_CONNECTIONSOCKETWINIMPL_H
