//
// Created by FNU Zhaluo on 9/25/26.
//

#ifndef TRDP_LISTENINGSOCKETPOSIXIMPL_H
#define TRDP_LISTENINGSOCKETPOSIXIMPL_H

#include "tcp/Socket.h"

namespace network {
class ListeningSocket::Impl {
public:
    static std::expected<std::unique_ptr<Impl>, int> Listen(uint16_t port);

    [[nodiscard]] int getSocket() const { return socketFd_; }

    // Blocks until a connection is pending (returns 0), abort() has been called (returns kAcceptAborted),
    // or polling fails (returns errno). Once it observes abort() it closes the listening socket before
    // returning, and every later call returns kAcceptAborted immediately.
    //
    // Only one thread may be in waitForConnection()/getSocket() at a time: that thread owns socketFd_.
    [[nodiscard]] int waitForConnection();

    // Thread-safe and idempotent; may be called from any thread. Only signals the accepting thread via
    // the wake pipe. It never touches socketFd_, so it cannot close the fd out from under accept().
    void abort();

    // Must not run concurrently with any other member function.
    ~Impl();
private:
    explicit Impl() = default;

    void closeListeningSocket();

    // Owned by the accepting thread (and the destructor). abort() must not touch it.
    int socketFd_ = -1;
    // Self-pipe: abort() writes to wakeFds_[1] so that poll() on wakeFds_[0] returns.
    // Both ends are created in Listen() and closed in the destructor only, so they are immutable in between.
    int wakeFds_[2] = {-1, -1};
    std::atomic<bool> aborted_{false};
};
}

#endif //TRDP_LISTENINGSOCKETPOSIXIMPL_H
