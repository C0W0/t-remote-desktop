#include "protocol/shared/Disconnect.h"

namespace network {
std::array<char, TransportHeader::networkSize()> serializeDisconnect() {
    // The shared message types have the same underlying value,
    // so ClientMessageType::Disconnect is valid for both client and server
    constexpr TransportHeader header{ClientMessageType::Disconnect, 0, 0};
    return serializeHeaderA(header);
}
}