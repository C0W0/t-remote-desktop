//
// Created by Terry on 2026-07-21.
//

#include "protocol/Transport.h"

std::array<char, sizeof(TransportHeader)> serializeHeaderA(const TransportHeader& header) {
    std::array<char, sizeof(TransportHeader)> result{};

    const auto typeVal = hostToNetwork(header.type);
    const auto flagsVal = hostToNetwork(header.flags);
    const auto lengthVal = hostToNetwork(header.length);

    memcpy(result.data(), &typeVal, sizeof(typeVal));
    memcpy(result.data() + sizeof(typeVal), &flagsVal, sizeof(flagsVal));
    memcpy(result.data() + sizeof(typeVal) + sizeof(flagsVal), &lengthVal, sizeof(lengthVal));

    return result;
}

std::vector<char> serializeHeaderV(const TransportHeader& header) {
    std::array<char, sizeof(TransportHeader)> result = serializeHeaderA(header);
    return {result.begin(), result.end()};
}

TransportHeader deserializeHeader(const std::span<const char> bytesData) {
    const char* dataPtr = bytesData.data();

    uint16_t typeValRaw;
    uint16_t flagsValRaw;
    uint32_t lengthValRaw;
    memcpy(&typeValRaw, dataPtr, sizeof(typeValRaw));
    memcpy(&flagsValRaw, dataPtr + sizeof(typeValRaw), sizeof(flagsValRaw));
    memcpy(&lengthValRaw, dataPtr + sizeof(typeValRaw) + sizeof(flagsValRaw), sizeof(lengthValRaw));

    return {
        .type = static_cast<MessageType>(hostToNetwork(typeValRaw)),
        .flags = hostToNetwork(flagsValRaw),
        .length = hostToNetwork(lengthValRaw)
    };
}

std::array<char, sizeof(TransportHeader)> serializeDisconnect() {
    constexpr TransportHeader header {.type = MessageType::Disconnect, .flags = 0, .length = 0};
    return serializeHeaderA(header);
}
