#include "protocol/shared/Message.h"
#include "protocol/Transport.h"

namespace network {
std::vector<char> serializeMessage(const std::string& message) {
    const TransportHeader header(ClientMessageType::Message, 0, message.size());
    std::vector<char> serialized = serializeHeaderV(header);
    serialized.insert(serialized.end(), message.begin(), message.end());
    return serialized;
}
}