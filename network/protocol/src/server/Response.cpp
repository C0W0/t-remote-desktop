#include "protocol/server/Response.h"
#include "protocol/Transport.h"

#include <cstring>

namespace network {
std::array<char, ServerRespMeta::networkSize()> serializeServerRespMeta(const ServerRespMeta& meta) {
    std::array<char, ServerRespMeta::networkSize()> result{};
    serializeServerRespMetaAt(meta, {result.begin(), result.end()});
    return result;
}

bool serializeServerRespMetaAt(const ServerRespMeta& meta, std::span<char> buffer) {
    if (buffer.size() < ServerRespMeta::networkSize()) {
        return false;
    }
    const auto statusVal = hostToNetwork(meta.status);
    const auto lenVal = hostToNetwork(meta.messageLength);
    memcpy(buffer.data(), &statusVal, sizeof(statusVal));
    memcpy(buffer.data() + sizeof(statusVal), &lenVal, sizeof(lenVal));
    return true;
}

std::vector<char> serializeServerResponse(ResponseStatus status, std::span<const char> message) {
    const TransportHeader respHeader{
        ServerMessageType::Response, 0, static_cast<uint32_t>(ServerRespMeta::networkSize()+message.size())
    };
    const ServerRespMeta meta { .status = status, .messageLength = static_cast<uint16_t>(message.size()) };

    std::vector<char> respSerialized = serializeHeaderV(respHeader);

    const uint32_t responseSerializedSize = respSerialized.size();
    respSerialized.resize(responseSerializedSize + ServerRespMeta::networkSize() + meta.messageLength);
    std::span<char> respSerializedSpan = respSerialized;
    respSerializedSpan = respSerializedSpan.subspan(responseSerializedSize);

    // cannot fail; discarding the return value
    serializeServerRespMetaAt(meta, respSerializedSpan);
    respSerializedSpan = respSerializedSpan.subspan(ServerRespMeta::networkSize());

    std::ranges::copy(message, respSerializedSpan.begin());

    return respSerialized;
}

std::expected<ServerRespMeta, std::string> deserializeServerRespMeta(std::span<const char> bytesData) {
    if (bytesData.size() < ServerRespMeta::networkSize()) {
        return std::unexpected("Insufficient data for ServerRespMeta");
    }

    const char* dataPtr = bytesData.data();

    uint16_t statusRaw;
    uint16_t messageLengthRaw;
    memcpy(&statusRaw, dataPtr, sizeof(statusRaw));
    memcpy(&messageLengthRaw, dataPtr + sizeof(statusRaw), sizeof(messageLengthRaw));

    if (networkToHost(statusRaw) > maxTypeValue<ResponseStatus>()) {
        return std::unexpected("Invalid ResponseStatus");
    }

    return ServerRespMeta {
        .status = static_cast<ResponseStatus>(networkToHost(statusRaw)),
        .messageLength = networkToHost(messageLengthRaw)
    };
}
}