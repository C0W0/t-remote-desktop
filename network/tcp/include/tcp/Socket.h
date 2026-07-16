//
// Created by Terry on 2026-07-10.
//

#ifndef TRDP_SOCKET_H
#define TRDP_SOCKET_H

#include <memory>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

namespace network {
class ConnectionSocket;
class ListeningSocket;

using ArcConnectionSocket = std::atomic<std::shared_ptr<ConnectionSocket>>;

struct AddrInfo {
    std::string address;
    uint16_t port;
};

class ConnectionSocket {
public:
    static ArcConnectionSocket toArcConnectionSocket(ConnectionSocket&& socket);
    // outAddrInfo is optional
    static std::expected<ConnectionSocket, int> Accept(const ListeningSocket& listeningSocket, AddrInfo* outAddrInfo);
    static std::expected<ConnectionSocket, int> Connect(const char* address, uint16_t port);

    ConnectionSocket(ConnectionSocket&& other) noexcept;
    ConnectionSocket& operator=(ConnectionSocket&& other) noexcept;
    ConnectionSocket(const ConnectionSocket& other) = delete;
    ConnectionSocket& operator=(const ConnectionSocket& other) = delete;
    ~ConnectionSocket();

    std::expected<int, int> recv(std::span<char> buffer);
    std::expected<void, int> send(std::string_view buffer);
    void close();

    class Impl;
private:
    ConnectionSocket();

    std::unique_ptr<Impl> pImpl_;
};

class ListeningSocket {
public:
    static std::expected<ListeningSocket, int> Listen(uint16_t port);

    ListeningSocket(ListeningSocket&& other) noexcept;
    ListeningSocket& operator=(ListeningSocket&& other) noexcept;
    ListeningSocket(const ListeningSocket& other) = delete;
    ListeningSocket& operator=(const ListeningSocket& other) = delete;
    ~ListeningSocket();

    void close();

    friend class ConnectionSocket::Impl;
private:
    ListeningSocket();

    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // network

#endif //TRDP_SOCKET_H
