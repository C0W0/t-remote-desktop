//
// Created by Terry on 9/25/26.
//


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <print>
#include <string>

#include "ListeningSocketPosixImpl.h"

using namespace network;

std::expected<std::unique_ptr<ListeningSocket::Impl>, int> ListeningSocket::Impl::Listen(uint16_t port) {
    addrinfo *result = nullptr;

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    int iResult = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result);
    if (iResult != 0) {
        std::println("getaddrinfo failed: {}", iResult);
        return std::unexpected(iResult);
    }

    const int listenSocketFd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocketFd < 0) {
        const int err = errno;
        std::println("Error at socket(): {}", err);
        freeaddrinfo(result);
        return std::unexpected(err);
    }

    // Setup the TCP listening socket
    iResult = bind(listenSocketFd, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (iResult == -1) {
        const int err = errno;
        std::println("bind failed with error: {}", err);
        freeaddrinfo(result);
        ::close(listenSocketFd);
        return std::unexpected(err);
    }
    freeaddrinfo(result);

    if (listen(listenSocketFd, SOMAXCONN) == -1) {
        const int err = errno;
        std::println("Listen failed with error: {}\n", err);
        ::close(listenSocketFd);
        return std::unexpected(err);
    }

    std::unique_ptr<ListeningSocket::Impl> socketImpl{new ListeningSocket::Impl{}};
    socketImpl->socketFd_ = listenSocketFd;
    return socketImpl;
}

void ListeningSocket::Impl::abort() {
    ::close(socketFd_);
    socketFd_ = -1;
}

ListeningSocket::Impl::~Impl() {
    std::println("Listening socket closed");
    abort();
}
