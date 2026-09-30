//
// Created by Terry on 2026-07-13.
//

#include "tcp/Socket.h"
#include "WindowsContext.h"
#include "ListeningSocketWinImpl.h"

#include <print>
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
        std::println("getaddrinfo failed: {}", iResult);
        return std::unexpected(iResult);
    }

    const SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        const int err = WSAGetLastError();
        std::println("Error at socket(): {}", err);
        freeaddrinfo(result);
        return std::unexpected(err);
    }

    // Setup the TCP listening socket
    iResult = bind(listenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (iResult == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println("bind failed with error: {}", err);
        freeaddrinfo(result);
        closesocket(listenSocket);
        return std::unexpected(err);
    }
    freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println("Listen failed with error: {}\n", err);
        closesocket(listenSocket);
        return std::unexpected(err);
    }

    // Both events are manual-reset and initially non-signalled.
    WSAEVENT acceptEvent = WSACreateEvent();
    if (acceptEvent == WSA_INVALID_EVENT) {
        const int err = WSAGetLastError();
        std::println("WSACreateEvent failed: {}", err);
        closesocket(listenSocket);
        return std::unexpected(err);
    }

    WSAEVENT abortEvent = WSACreateEvent();
    if (abortEvent == WSA_INVALID_EVENT) {
        const int err = WSAGetLastError();
        std::println("WSACreateEvent failed: {}", err);
        WSACloseEvent(acceptEvent);
        closesocket(listenSocket);
        return std::unexpected(err);
    }

    // Have Winsock signal acceptEvent while a connection is pending. This also switches the listening
    // socket to non-blocking mode, so accept() can never block after the event fires (e.g. the pending
    // connection was reset in between).
    if (WSAEventSelect(listenSocket, acceptEvent, FD_ACCEPT) == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        std::println("WSAEventSelect failed: {}", err);
        WSACloseEvent(abortEvent);
        WSACloseEvent(acceptEvent);
        closesocket(listenSocket);
        return std::unexpected(err);
    }

    std::unique_ptr<ListeningSocket::Impl> socketImpl{new ListeningSocket::Impl{}};
    socketImpl->socket_ = listenSocket;
    socketImpl->acceptEvent_ = acceptEvent;
    socketImpl->abortEvent_ = abortEvent;
    return std::move(socketImpl);
}

int ListeningSocket::Impl::waitForConnection() {
    // abortEvent_ is first: if both are signalled, WSAWaitForMultipleEvents reports the lowest index, so
    // an abort wins over a pending connection (same priority as the POSIX implementation).
    const WSAEVENT events[2] = {abortEvent_, acceptEvent_};

    while (true) {
        const DWORD result = WSAWaitForMultipleEvents(2, events, FALSE, WSA_INFINITE, FALSE);
        if (result == WSA_WAIT_FAILED) {
            return WSAGetLastError();
        }

        // We are the only user of the listening socket, so it is safe to close it here: no other thread
        // can be inside accept() on it.
        if (result == WSA_WAIT_EVENT_0) {
            closeListeningSocket();
            return kAcceptAborted;
        }

        // acceptEvent_ fired. Reading the network events also resets the (manual-reset) event.
        WSANETWORKEVENTS networkEvents{};
        if (WSAEnumNetworkEvents(socket_, acceptEvent_, &networkEvents) == SOCKET_ERROR) {
            return WSAGetLastError();
        }
        if ((networkEvents.lNetworkEvents & FD_ACCEPT) != 0) {
            // 0 means a connection is pending; otherwise the listening socket itself failed.
            return networkEvents.iErrorCode[FD_ACCEPT_BIT];
        }
        // Spurious wake-up with no FD_ACCEPT recorded: keep waiting.
    }
}

void ListeningSocket::Impl::abort() {
    if (aborted_.exchange(true)) {
        return;
    }

    // Wake the accepting thread. The event is never reset, so it stays signalled and any later
    // waitForConnection() also returns kAcceptAborted. abortEvent_ is immutable until the destructor.
    WSASetEvent(abortEvent_);
}

void ListeningSocket::Impl::closeListeningSocket() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

ListeningSocket::Impl::~Impl() {
    std::println("Listening socket closed");
    closeListeningSocket();
    if (acceptEvent_ != WSA_INVALID_EVENT) {
        WSACloseEvent(acceptEvent_);
        acceptEvent_ = WSA_INVALID_EVENT;
    }
    if (abortEvent_ != WSA_INVALID_EVENT) {
        WSACloseEvent(abortEvent_);
        abortEvent_ = WSA_INVALID_EVENT;
    }
}


