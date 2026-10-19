#include "protocol/client/Control.h"

#include <cstring>

namespace network {
bool serializeControlMetaTo(ControlMeta meta, std::span<char> buffer) {
    if (buffer.size() < ControlMeta::networkSize()) {
        return false;
    }
    const auto typeVal = hostToNetwork(meta.type);
    memcpy(buffer.data(), &typeVal, sizeof(typeVal));
    return true;
}

std::expected<ControlMeta, std::string> deserializeControlMeta(std::span<char> buffer) {
    if (buffer.size() < ControlMeta::networkSize()) {
        return std::unexpected("Insufficient data for AuthMeta");
    }

    uint16_t typeRaw;
    memcpy(&typeRaw, buffer.data(), sizeof(typeRaw));
    typeRaw = networkToHost(typeRaw);
    constexpr auto maxTypeVal = maxTypeValue<ControlType>();
    if (typeRaw > maxTypeVal) {
        return std::unexpected("Invalid ControlType");
    }

    ControlMeta meta { .type = static_cast<ControlType>(typeRaw) };
    return meta;
}

bool serializeKeyEventPayloadTo(KeyEventPayload payload, std::span<char> buffer) {
    if (buffer.size() < KeyEventPayload::networkSize()) {
        return false;
    }
    const auto keyVal = hostToNetwork(payload.key);
    memcpy(buffer.data(), &keyVal, sizeof(keyVal));
    return true;
}

std::array<char, TransportHeader::networkSize() + ControlMeta::networkSize() + KeyEventPayload::networkSize()>
serializeKeyEvent(KeyboardEvents type, KeyCode key) {
    std::array<char, TransportHeader::networkSize() + ControlMeta::networkSize() + KeyEventPayload::networkSize()> result{};
    const auto header = serializeHeaderA(
        TransportHeader{ClientMessageType::Control, 0, ControlMeta::networkSize() + KeyEventPayload::networkSize()}
    );
    ControlMeta meta = {toControlType(type)};
    KeyEventPayload payload{key};
    memcpy(result.data(), header.data(), header.size());
    serializeControlMetaTo(meta, {result.data() + header.size(), ControlMeta::networkSize()});
    serializeKeyEventPayloadTo(payload, {result.data() + header.size() + ControlMeta::networkSize(), KeyEventPayload::networkSize()});
    return result;
}
}