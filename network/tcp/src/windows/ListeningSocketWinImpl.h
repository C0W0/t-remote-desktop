//
// Created by Terry on 2026-07-13.
//

#ifndef TRDP_LISTENINGSOCKETWINIMPL_H
#define TRDP_LISTENINGSOCKETWINIMPL_H

#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "tcp/Socket.h"

namespace network {
class ListeningSocket::Impl {
public:
    static std::expected<std::unique_ptr<Impl>, int> Listen(uint16_t port);

    [[nodiscard]] SOCKET getSocket() const { return socket_; }

    // Blocks until a connection is pending (returns 0), abort() has been called (returns kAcceptAborted),
    // or waiting fails (returns the WSA error). Once it observes abort() it closes the listening socket
    // before returning, and every later call returns kAcceptAborted immediately.
    //
    // Only one thread may be in waitForConnection()/getSocket() at a time: that thread owns socket_.
    [[nodiscard]] int waitForConnection();

    // Thread-safe and idempotent; may be called from any thread. Only signals the accepting thread via
    // abortEvent_. It never touches socket_, so it cannot close the socket out from under accept().
    void abort();

    // Must not run concurrently with any other member function.
    ~Impl();
private:
    explicit Impl() = default;

    void closeListeningSocket();

    // Owned by the accepting thread (and the destructor). abort() must not touch it.
    SOCKET socket_ = INVALID_SOCKET;
    // Signalled by Winsock (WSAEventSelect/FD_ACCEPT) while a connection is pending.
    // Created in Listen() and closed in the destructor only, so immutable in between.
    WSAEVENT acceptEvent_ = WSA_INVALID_EVENT;
    // Manual-reset event set by abort(); never reset, so it stays signalled like the POSIX wake pipe.
    WSAEVENT abortEvent_ = WSA_INVALID_EVENT;
    std::atomic<bool> aborted_{false};
};
}

#endif //TRDP_LISTENINGSOCKETWINIMPL_H
