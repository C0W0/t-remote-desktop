//
// Created by Terry on 2026-07-10.
//

#include "tcp/Socket.h"

#ifdef WIN32
#include "windows/ConnectionSocketWinImpl.h"
#include "windows/ListeningSocketWinImpl.h"
#endif


using namespace network;

std::expected<ListeningSocket, int> ListeningSocket::CreateSocket(uint16_t port) {
    auto result = ListeningSocket::Impl::Listen(port);
    return std::move(result).transform([](std::unique_ptr<ListeningSocket::Impl>&& impl) {
        ListeningSocket socket;
        socket.pImpl_ = std::move(impl);
        return socket;
    });
}

ListeningSocket::ListeningSocket() = default;
ListeningSocket::ListeningSocket(ListeningSocket&& other) noexcept = default;
ListeningSocket& ListeningSocket::operator=(ListeningSocket&& other) noexcept = default;
ListeningSocket::~ListeningSocket() = default;


std::expected<ConnectionSocket, int> ConnectionSocket::CreateSocket(const ListeningSocket& listeningSocket) {
    auto result = ConnectionSocket::Impl::Accept(listeningSocket);
    return std::move(result).transform([](std::unique_ptr<ConnectionSocket::Impl>&& impl) {
        ConnectionSocket socket;
        socket.pImpl_ = std::move(impl);
        return socket;
    });
}

ConnectionSocket::ConnectionSocket() = default;
ConnectionSocket::ConnectionSocket(ConnectionSocket&& other) noexcept = default;
ConnectionSocket& ConnectionSocket::operator=(ConnectionSocket&& other) noexcept = default;
ConnectionSocket::~ConnectionSocket() = default;

