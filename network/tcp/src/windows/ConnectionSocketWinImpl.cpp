//
// Created by Terry on 2026-07-10.
//

#include "ConnectionSocketWinImpl.h"
#include "ListeningSocketWinImpl.h"
#include "WindowsContext.h"
#include "tcp/Socket.h"

#include <cstring>
#include <print>

using namespace network;

std::expected<std::unique_ptr<ConnectionSocket::Impl>, int>
ConnectionSocket::Impl::Accept(const ListeningSocket& listeningSocket, AddrInfo* outAddrInfo) {
    auto& listener = *listeningSocket.pImpl_;

    sockaddr_in clientAddr {};
    SOCKET clientSocket = INVALID_SOCKET;
    while (true) {
        // Blocks until a connection is pending or ListeningSocket::close() is called.
        if (const int err = listener.waitForConnection(); err != 0) {
            if (err != kAcceptAborted) {
                std::println("accept failed: {}", err);
            }
            return std::unexpected(err);
        }

        int addrLen = sizeof(clientAddr);
        clientSocket = accept(
            listener.getSocket(),
            outAddrInfo != nullptr ? reinterpret_cast<sockaddr*>(&clientAddr) : nullptr,
            outAddrInfo != nullptr ? &addrLen : nullptr
        );
        if (clientSocket != INVALID_SOCKET) {
            break;
        }

        // Transient: the pending connection may have gone away between the event and accept().
        const int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAECONNRESET || err == WSAEINTR) {
            continue;
        }
        std::println("accept failed: {}", err);
        return std::unexpected(err);
    }

    // An accepted socket inherits the listening socket's properties: its WSAEventSelect registration and
    // the non-blocking mode that comes with it. Connection sockets use blocking I/O, so undo both.
    if (WSAEventSelect(clientSocket, nullptr, 0) == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println("failed to cancel event selection on accepted socket: {}", err);
        closesocket(clientSocket);
        return std::unexpected(err);
    }
    u_long blockingMode = 0;
    if (ioctlsocket(clientSocket, FIONBIO, &blockingMode) == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println("failed to set accepted socket to blocking: {}", err);
        closesocket(clientSocket);
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
        std::println("getaddrinfo failed: {}", iResult);
        return std::unexpected(iResult);
    }

    SOCKET connectSocket{};
    addrinfo* originalResultPtr = result;
    bool connected = false;
    for(; result != nullptr; result = result->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            const int err = WSAGetLastError();
            std::println("socket failed with error: {}", err);
            freeaddrinfo(originalResultPtr);
            return std::unexpected(err);
        }

        // Connect to server.
        iResult = connect(connectSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
        if (iResult != SOCKET_ERROR) {
            connected = true;
            break;
        }
        closesocket(connectSocket);
    }

    freeaddrinfo(originalResultPtr);

    if (!connected) {
        const int err = WSAGetLastError();
        std::println("socket failed with error: {}", err);
        return std::unexpected(err);
    }

    std::unique_ptr<ConnectionSocket::Impl> socketImpl{new ConnectionSocket::Impl{}};
    socketImpl->socket_ = connectSocket;
    return std::move(socketImpl);
}

std::expected<int, int> ConnectionSocket::Impl::recv(std::span<char> buffer) {
    const int bytesRecv = ::recv(socket_, buffer.data(), buffer.size(), 0);

    // connection closed
    if (bytesRecv == 0) {
        std::println("connection closed");
        return std::unexpected(0);
    }
    // error
    if (bytesRecv < 0) {
        const int err = WSAGetLastError();
        std::println("recv failed: {}", err);
        close();
        return std::unexpected(err);
    }

    return bytesRecv;
}

std::expected<int, int> ConnectionSocket::Impl::send(std::span<const char> buffer) {
    const int iSendResult = ::send(socket_, buffer.data(), buffer.size(), 0);
    if (iSendResult == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println("send failed: {}", err);
        close();
        return std::unexpected(err);
    }
    std::println("Bytes sent: {}", iSendResult);
    return iSendResult;
}

void ConnectionSocket::Impl::close() {
    if (closed_.exchange(true)) {
        return;
    }
    std::println("Connection socket closed");

    // Signal only: the socket handle stays open. Releasing it here would let the OS hand the same handle value
    // to the next socket while another thread still uses this one, so that thread would read from / write to the
    // wrong connection. The handle is released in the destructor, when nobody can be using it any more.
    const int iResult = shutdown(socket_, SD_BOTH);
    if (iResult == SOCKET_ERROR) {
        std::println("shutdown failed: {}", WSAGetLastError());
    }

    // On POSIX shutdown() is what wakes a thread blocked in recv()/send(). On Windows that is not guaranteed for
    // a call that is already pending, so also cancel any I/O in flight on this socket, whichever thread issued it.
    // The cancelled call returns with an error (typically WSAEINTR). A FALSE result with ERROR_NOT_FOUND just
    // means nothing was pending, which is fine, so the result is ignored.
    CancelIoEx(reinterpret_cast<HANDLE>(socket_), nullptr);
}

ConnectionSocket::Impl::~Impl() {
    std::println("Connection socket dropped");
    // No-op if close() already ran; otherwise shuts the connection down first.
    close();
    // The only place the socket handle is released.
    closesocket(socket_);
}
