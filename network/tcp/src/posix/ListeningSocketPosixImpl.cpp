//
// Created by Terry on 9/25/26.
//

#include "Util.h"
#include "ListeningSocketPosixImpl.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <netdb.h>

#include <print>
#include <string>

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

    // Non-blocking so that accept() can never block after poll() reports readiness
    // (e.g. the pending connection was reset in between).
    if (const int err = setNonBlockingCloexec(listenSocketFd); err != 0) {
        std::println("failed to configure listening socket: {}", err);
        ::close(listenSocketFd);
        return std::unexpected(err);
    }

    int wakeFds[2] = {-1, -1};
    if (const int err = makeWakePipe(wakeFds); err != 0) {
        std::println("failed to create wake pipe: {}", err);
        ::close(listenSocketFd);
        return std::unexpected(err);
    }

    std::unique_ptr<ListeningSocket::Impl> socketImpl{new ListeningSocket::Impl{}};
    socketImpl->socketFd_ = listenSocketFd;
    socketImpl->wakeFds_[0] = wakeFds[0];
    socketImpl->wakeFds_[1] = wakeFds[1];
    return socketImpl;
}

int ListeningSocket::Impl::waitForConnection() {
    pollfd fds[2] = {
        {.fd = socketFd_, .events = POLLIN, .revents = 0},
        {.fd = wakeFds_[0], .events = POLLIN, .revents = 0},
    };

    while (true) {
        if (poll(fds, 2, -1) == -1) {
            if (errno == EINTR) {
                continue;
            }
            return errno;
        }

        // Check the wake pipe first. We are the only user of the listening socket, so it is safe to
        // close it here: no other thread can be inside accept() on it.
        if (fds[1].revents != 0) {
            closeListeningSocket();
            return kAcceptAborted;
        }
        if ((fds[0].revents & POLLIN) != 0) {
            return 0;
        }
        if ((fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            return EBADF;
        }
    }
}

void ListeningSocket::Impl::abort() {
    if (aborted_.exchange(true)) {
        return;
    }

    // Wake the accepting thread. The pipe is never drained, so it stays readable and any later
    // waitForConnection() also returns kAcceptAborted. wakeFds_ is immutable until the destructor.
    constexpr char wake = 1;
    ssize_t written;
    do {
        written = ::write(wakeFds_[1], &wake, 1);
    } while (written == -1 && errno == EINTR);
}

void ListeningSocket::Impl::closeListeningSocket() {
    if (socketFd_ != -1) {
        ::close(socketFd_);
        socketFd_ = -1;
    }
}

ListeningSocket::Impl::~Impl() {
    std::println("Listening socket closed");
    closeListeningSocket();
    for (int& fd : wakeFds_) {
        if (fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }
}
