//
// Created by Terry on 2026-07-13.
//

#ifndef TRDP_LISTENINGSOCKETWINIMPL_H
#define TRDP_LISTENINGSOCKETWINIMPL_H

#include "tcp/Socket.h"
#include <winsock2.h>
#include <ws2tcpip.h>

namespace network {
class ListeningSocket::Impl {
public:
    static std::expected<std::unique_ptr<Impl>, int> Listen(uint16_t port);

    // DO NOT USE THE CONSTRUCTOR. Call `Listen` to create a socket instead.
    explicit Impl() = default;
private:
    SOCKET socket_ = INVALID_SOCKET;
};
}

#endif //TRDP_LISTENINGSOCKETWINIMPL_H
