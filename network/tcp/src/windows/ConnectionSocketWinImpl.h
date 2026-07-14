//
// Created by Terry on 2026-07-13.
//

#ifndef TRDP_CONNECTIONSOCKETWINIMPL_H
#define TRDP_CONNECTIONSOCKETWINIMPL_H

#include "tcp/Socket.h"

namespace network {
class ConnectionSocket::Impl {
public:
    static std::expected<std::unique_ptr<Impl>, int> Accept(const ListeningSocket& listeningSocket);
};
}

#endif //TRDP_CONNECTIONSOCKETWINIMPL_H
