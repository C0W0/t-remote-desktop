//
// Created by Terry on 2026-07-10.
//

#include <iostream>
#include <ostream>

#include "tcp/Socket.h"
#include "WindowsContext.h"
#include "ConnectionSocketWinImpl.h"
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

std::expected<std::unique_ptr<ConnectionSocket::Impl>, int>
ConnectionSocket::Impl::Connect(const char* address, const uint16_t port) {
    WinsockInitializer::Initialize();

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* result = nullptr;

    // Resolve the local address and port to be used by the server
    int iResult = getaddrinfo(address, std::to_string(port).c_str(), &hints, &result);
    if (iResult != 0) {
        std::println(std::cout, "getaddrinfo failed: {}", iResult);
        return std::unexpected(iResult);
    }

    SOCKET connectSocket{};
    addrinfo* originalResultPtr = result;
    for(; result != nullptr; result = result->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            const int err = WSAGetLastError();
            std::println(std::cout, "socket failed with error: {}", err);
            freeaddrinfo(originalResultPtr);
            return std::unexpected(err);
        }

        // Connect to server.
        iResult = connect(connectSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
        if (iResult != SOCKET_ERROR) {
            break;
        }
        closesocket(connectSocket);
    }

    freeaddrinfo(originalResultPtr);

    std::unique_ptr<ConnectionSocket::Impl> socketImpl{new ConnectionSocket::Impl{}};
    socketImpl->socket_ = connectSocket;
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

std::expected<int, int> ConnectionSocket::Impl::send(std::string_view buffer) {
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
    socket_ = INVALID_SOCKET;
    closed_ = true;
}

ConnectionSocket::Impl::~Impl() {
    std::println(std::cout, "Connection socket dropped");
    if (!closed_) {
        close();
    }
}
