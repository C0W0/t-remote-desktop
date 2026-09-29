//
// Created by Terry on 9/25/26.
//

#ifndef TRDP_CONNECTIONSOCKETWINIMPL_H
#define TRDP_CONNECTIONSOCKETWINIMPL_H

#include <span>

#include "tcp/Socket.h"


namespace network {
class ConnectionSocket::Impl {
public:
    static std::expected<std::unique_ptr<Impl>, int> Accept(const ListeningSocket& listeningSocket, AddrInfo* outAddrInfo);
    static std::expected<std::unique_ptr<Impl>, int> Connect(const char* address, uint16_t port);

    ~Impl();

    std::expected<int, int> recv(std::span<char> buffer);
    std::expected<int, int> send(std::span<const char> buffer);

    void close();

private:
    explicit Impl() = default;

    int socketFd_ = -1;
    bool closed_ = false;
};
}


#endif //TRDP_CONNECTIONSOCKETWINIMPL_H
