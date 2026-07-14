//
// Created by Terry on 2026-07-10.
//

#ifndef TRDP_SOCKET_H
#define TRDP_SOCKET_H

#include <memory>
#include <cstdint>
#include <expected>

namespace network {

class ListeningSocket {
public:
    static std::expected<ListeningSocket, int> CreateSocket(uint16_t port);

    ListeningSocket(ListeningSocket&& other) noexcept;
    ListeningSocket& operator=(ListeningSocket&& other) noexcept;
    ListeningSocket(const ListeningSocket& other) = delete;
    ListeningSocket& operator=(const ListeningSocket& other) = delete;
    ~ListeningSocket();

private:
    ListeningSocket();

    class Impl;
    std::unique_ptr<Impl> pImpl_;
};
class ConnectionSocket {
public:
    static std::expected<ConnectionSocket, int> CreateSocket(const ListeningSocket& listeningSocket);

    ConnectionSocket(ConnectionSocket&& other) noexcept;
    ConnectionSocket& operator=(ConnectionSocket&& other) noexcept;
    ConnectionSocket(const ConnectionSocket& other) = delete;
    ConnectionSocket& operator=(const ConnectionSocket& other) = delete;
    ~ConnectionSocket();

private:
    ConnectionSocket();

    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // network

#endif //TRDP_SOCKET_H
