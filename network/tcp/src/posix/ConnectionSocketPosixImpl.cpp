//
// Created by Terry on 9/25/26.
//


#include <print>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>


#include "tcp/Socket.h"
#include "ConnectionSocketPosixImpl.h"

#include <cstring>

#include "ListeningSocketPosixImpl.h"

using namespace network;

namespace {
// Sending to a peer that has already closed raises SIGPIPE, which terminates the process by default.
// macOS/BSD: suppress per-socket via SO_NOSIGPIPE. Linux: suppress per-call via MSG_NOSIGNAL (see send()).
// Returns 0 on success, otherwise errno.
int suppressSigPipe([[maybe_unused]] const int fd) {
#ifdef SO_NOSIGPIPE
    constexpr int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enable, sizeof(enable)) == -1) {
        return errno;
    }
#endif
    return 0;
}

#ifdef MSG_NOSIGNAL
constexpr int kSendFlags = MSG_NOSIGNAL;
#else
constexpr int kSendFlags = 0;
#endif

// The listening socket is non-blocking, and on BSD/macOS accepted sockets inherit that flag.
// Connection sockets are used with blocking I/O, so clear it. Returns 0 on success, otherwise errno.
int setBlocking(const int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        return errno;
    }
    return 0;
}
}

std::expected<std::unique_ptr<ConnectionSocket::Impl>, int>
ConnectionSocket::Impl::Accept(const ListeningSocket& listeningSocket, AddrInfo* outAddrInfo) {
    auto& listener = *listeningSocket.pImpl_;

    sockaddr_in clientAddr{};
    int clientSocketFd = -1;
    while (true) {
        // Blocks until a connection is pending or ListeningSocket::close() is called.
        if (const int err = listener.waitForConnection(); err != 0) {
            if (err != kAcceptAborted) {
                std::println("accept failed: {}", err);
            }
            return std::unexpected(err);
        }

        socklen_t addrLen = sizeof(clientAddr);
        clientSocketFd = accept(
            listener.getSocket(),
            outAddrInfo != nullptr ? reinterpret_cast<sockaddr *>(&clientAddr) : nullptr,
            outAddrInfo != nullptr ? &addrLen : nullptr
        );
        if (clientSocketFd >= 0) {
            break;
        }

        // Transient: the pending connection may have gone away between poll() and accept().
        const int err = errno;
        if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK || err == ECONNABORTED) {
            continue;
        }
        std::println("accept failed: {}", err);
        return std::unexpected(err);
    }

    if (const int err = setBlocking(clientSocketFd); err != 0) {
        std::println("failed to set accepted socket to blocking: {}", err);
        ::close(clientSocketFd);
        return std::unexpected(err);
    }

    if (const int err = suppressSigPipe(clientSocketFd); err != 0) {
        std::println("failed to set SO_NOSIGPIPE: {}", err);
        ::close(clientSocketFd);
        return std::unexpected(err);
    }

    if (outAddrInfo != nullptr) {
        outAddrInfo->address.resize(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &clientAddr.sin_addr, outAddrInfo->address.data(), INET_ADDRSTRLEN);
        outAddrInfo->address.resize(std::strlen(outAddrInfo->address.data()));
        outAddrInfo->port = ntohs(clientAddr.sin_port);
    }

    std::unique_ptr<ConnectionSocket::Impl> socketImpl{new ConnectionSocket::Impl{}};
    socketImpl->socketFd_ = clientSocketFd;
    return socketImpl;
}

std::expected<std::unique_ptr<ConnectionSocket::Impl>, int>
ConnectionSocket::Impl::Connect(const char* address, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo* result = nullptr;

    // Resolve the local address and port to be used by the server
    int iResult = getaddrinfo(address, std::to_string(port).c_str(), &hints, &result);
    if (iResult != 0) {
        std::println("getaddrinfo failed: {}", iResult);
        return std::unexpected(iResult);
    }

    int connectSocket{};
    addrinfo* originalResultPtr = result;
    bool connected = false;
    for(; result != nullptr; result = result->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (connectSocket == -1) {
            const int err = errno;
            std::println("socket failed with error: {}", err);
            freeaddrinfo(originalResultPtr);
            return std::unexpected(err);
        }

        if (const int err = suppressSigPipe(connectSocket); err != 0) {
            std::println("failed to set SO_NOSIGPIPE: {}", err);
            ::close(connectSocket);
            freeaddrinfo(originalResultPtr);
            return std::unexpected(err);
        }

        // Connect to server.
        iResult = connect(connectSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
        if (iResult == 0) {
            connected = true;
            break;
        }
        ::close(connectSocket);
        connectSocket = -1;
    }
    
    freeaddrinfo(originalResultPtr);

    if (!connected) {
        const int err = errno;
        std::println("socket failed with error: {}", err);
        return std::unexpected(err);
    }

    std::unique_ptr<ConnectionSocket::Impl> socketImpl{new ConnectionSocket::Impl{}};
    socketImpl->socketFd_ = connectSocket;

    return socketImpl;
}

std::expected<int, int> ConnectionSocket::Impl::recv(std::span<char> buffer) {
    const ssize_t bytesRecv = ::recv(this->socketFd_, buffer.data(), buffer.size(), 0);

    if (bytesRecv == 0) {
        std::println("connection closed");
        return std::unexpected(0);
    }

    // error
    if (bytesRecv < 0) {
        const int err = errno;
        std::println("recv failed: {}", err);
        close();
        return std::unexpected(err);
    }

    return bytesRecv;
}

std::expected<int, int> ConnectionSocket::Impl::send(std::span<const char> buffer) {
    const int iSendResult = ::send(socketFd_, buffer.data(), buffer.size(), kSendFlags);
    if (iSendResult == -1) {
        const int err = errno;
        std::println("send failed: {}", err);
        close();
        return std::unexpected(err);
    }
    std::println("Bytes sent: {}", iSendResult);
    return iSendResult;
}

void ConnectionSocket::Impl::close() {
    std::println("Connection socket closed");
    const int iResult = shutdown(socketFd_, SHUT_WR);
    if (iResult == -1) {
        std::println("shutdown failed: {}", errno);
    }
    ::close(socketFd_);
    socketFd_ = -1;
    closed_ = true;
}

ConnectionSocket::Impl::~Impl() {
    std::println("Connection socket dropped");
    if (!closed_) {
        close();
    }
}
