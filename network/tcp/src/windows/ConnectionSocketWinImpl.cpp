//
// Created by Terry on 2026-07-10.
//

#include "tcp/Socket.h"
#include "WindowsContext.h"
#include "ConnectionSocketWinImpl.h"

#include <iostream>
#include <ostream>

#include "ListeningSocketWinImpl.h"

using namespace network;

std::expected<std::unique_ptr<ConnectionSocket::Impl>, int>
ConnectionSocket::Impl::Accept(const ListeningSocket& listeningSocket, AddrInfo* outAddrInfo) {
    sockaddr_in clientAddr {};
    SOCKET clientSocket {};
    if (outAddrInfo != nullptr) {
        int addrLen = sizeof(clientAddr);
        clientSocket = accept(listeningSocket.pImpl_->getSocket(), static_cast<sockaddr*>(static_cast<void*>(&clientAddr)), &addrLen);
    }
    else {
        clientSocket = accept(listeningSocket.pImpl_->getSocket(), nullptr, nullptr);
    }

    if (clientSocket == INVALID_SOCKET) {
        const int err = WSAGetLastError();
        std::println(std::cout, "accept failed: {}", err);
        return std::unexpected(err);
    }

    if (outAddrInfo != nullptr) {
        outAddrInfo->address.resize(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &clientAddr.sin_addr, outAddrInfo->address.data(), INET_ADDRSTRLEN);
        outAddrInfo->address.resize(std::strlen(outAddrInfo->address.data()));
        outAddrInfo->port = ntohs(clientAddr.sin_port);
    }

    std::unique_ptr<ConnectionSocket::Impl> socketImpl{new ConnectionSocket::Impl{}};
    socketImpl->socket_ = clientSocket;
    return std::move(socketImpl);
}

std::expected<int, int> ConnectionSocket::Impl::recv(std::span<char> buffer) {
    const int bytesRecv = ::recv(socket_, buffer.data(), buffer.size(), 0);

    // connection closed
    if (bytesRecv == 0) {
        std::println(std::cout, "connection closed");
        return std::unexpected(0);
    }
    // error
    if (bytesRecv < 0) {
        const int err = WSAGetLastError();
        std::println(std::cout, "recv failed: {}", err);
        close();
        return std::unexpected(err);
    }

    return bytesRecv;
}

std::expected<void, int> ConnectionSocket::Impl::send(std::string_view buffer) {
    int bytesSent = 0;
    do {
        auto result = sendImpl(buffer);
        if (!result) {
            return std::unexpected(result.error());
        }
        bytesSent = result.value();
        buffer = buffer.substr(bytesSent);
    } while (!buffer.empty() && bytesSent != 0);

    return {};
}

std::expected<int, int> ConnectionSocket::Impl::sendImpl(std::string_view buffer) {
    const int iSendResult = ::send(socket_, buffer.data(), buffer.size(), 0);
    if (iSendResult == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println(std::cout, "send failed: {}", err);
        close();
        return std::unexpected(err);
    }
    std::println(std::cout, "Bytes sent: {}", iSendResult);
    return iSendResult;
}

void ConnectionSocket::Impl::close() {
    std::println(std::cout, "Connection socket closed");
    const int iResult = shutdown(socket_, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::println(std::cout, "shutdown failed: {}", WSAGetLastError());
    }
    closesocket(socket_);
    closed_ = true;
}

ConnectionSocket::Impl::~Impl() {
    std::println(std::cout, "Connection socket dropped");
    if (!closed_) {
        close();
    }
}
