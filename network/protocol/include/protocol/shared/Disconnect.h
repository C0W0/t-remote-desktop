//
// Created by Terry Zha on 10/14/26.
//

#ifndef TRDP_DISCONNECT_H
#define TRDP_DISCONNECT_H

#include "protocol/Transport.h"

namespace network {
std::array<char, TransportHeader::networkSize()> serializeDisconnect();
}

#endif //TRDP_DISCONNECT_H
