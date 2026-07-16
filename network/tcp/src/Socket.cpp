//
// Created by Terry on 2026-07-10.
//

#include "tcp/Socket.h"

#ifdef WIN32
#include "windows/ConnectionSocketWinImpl.h"
#include "windows/ListeningSocketWinImpl.h"
#endif


using namespace network;

std::expected<ListeningSocket, int> ListeningSocket::Listen(uint16_t port) {
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

void ListeningSocket::close() {
    pImpl_->abort();
}


ConnectionSocket::ConnectionSocket() = default;
ConnectionSocket::ConnectionSocket(ConnectionSocket&& other) noexcept = default;
ConnectionSocket& ConnectionSocket::operator=(ConnectionSocket&& other) noexcept = default;
ConnectionSocket::~ConnectionSocket() = default;

ArcConnectionSocket ConnectionSocket::toArcConnectionSocket(ConnectionSocket&& socket) {
    return std::make_shared<ConnectionSocket>(std::move(socket));
}

std::expected<ConnectionSocket, int> ConnectionSocket::Accept(const ListeningSocket& listeningSocket, AddrInfo* outAddrInfo) {
    auto result = ConnectionSocket::Impl::Accept(listeningSocket, outAddrInfo);
    return std::move(result).transform([](std::unique_ptr<ConnectionSocket::Impl>&& impl) {
        ConnectionSocket socket;
        socket.pImpl_ = std::move(impl);
        return socket;
    });
}

std::expected<ConnectionSocket, int> ConnectionSocket::Connect(const char* address, uint16_t port) {
    auto result = ConnectionSocket::Impl::Connect(address, port);
    return std::move(result).transform([](std::unique_ptr<ConnectionSocket::Impl>&& impl) {
        ConnectionSocket socket;
        socket.pImpl_ = std::move(impl);
        return socket;
    });
}

std::expected<int, int> ConnectionSocket::recv(std::span<char> buffer) {
    return pImpl_->recv(buffer);
}

std::expected<void, int> ConnectionSocket::send(std::string_view buffer) {
    return pImpl_->send(buffer);
}

void ConnectionSocket::close() {
    pImpl_->close();
}

