//
// Created by Terry on 2026-07-13.
//

#ifndef TRDP_CONNECTIONSOCKETWINIMPL_H
#define TRDP_CONNECTIONSOCKETWINIMPL_H

#include <atomic>
#include <span>
#include <winsock2.h>
#include <ws2tcpip.h>

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

    // Never changes after construction and is only released by the destructor, so any thread may read it.
    SOCKET socket_ = INVALID_SOCKET;
    // Set by the first close(); later calls do nothing.
    std::atomic<bool> closed_ = false;
};
}

#endif //TRDP_CONNECTIONSOCKETWINIMPL_H
