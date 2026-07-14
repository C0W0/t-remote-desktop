//
// Created by Terry on 2026-07-13.
//

#include "tcp/Socket.h"
#include "WindowsContext.h"
#include "ListeningSocketWinImpl.h"

#include <iostream>
#include <ostream>
#include <string>

using namespace network;

std::expected<std::unique_ptr<ListeningSocket::Impl>, int> ListeningSocket::Impl::Listen(uint16_t port) {
    WinsockInitializer::Initialize();

    addrinfo *result = nullptr;

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    int iResult = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result);
    if (iResult != 0) {
        std::println(std::cout, "getaddrinfo failed: {}", iResult);
        return std::unexpected(iResult);
    }

    const SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        const int err = WSAGetLastError();
        std::println(std::cout, "Error at socket(): {}", err);
        freeaddrinfo(result);
        return std::unexpected(err);
    }

    // Setup the TCP listening socket
    iResult = bind(listenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (iResult == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println(std::cout, "bind failed with error: {}", err);
        freeaddrinfo(result);
        closesocket(listenSocket);
        return std::unexpected(err);
    }
    freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println(std::cout,"Listen failed with error: {}\n", err);
        closesocket(listenSocket);
        return std::unexpected(err);
    }

    std::unique_ptr<ListeningSocket::Impl> socketImpl{new ListeningSocket::Impl{}};
    socketImpl->socket_ = listenSocket;
    return std::move(socketImpl);
}

ListeningSocket::Impl::~Impl() {
    std::println(std::cout, "Listening socket closed");
    closesocket(socket_);
}


