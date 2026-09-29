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

    void abort();

    ~Impl();
private:
    explicit Impl() = default;
    int socketFd_ = -1;
};
}

#endif //TRDP_LISTENINGSOCKETPOSIXIMPL_H
