//
// Created by Terry on 2026-07-21.
//

#include "protocol/Transport.h"

#include <cstring>

namespace network {
std::array<char, TransportHeader::networkSize()> serializeHeaderA(const TransportHeader& header) {
    std::array<char, TransportHeader::networkSize()> result{};

    const auto typeVal = hostToNetwork(header.type);
    const auto flagsVal = hostToNetwork(header.flags);
    const auto lengthVal = hostToNetwork(header.length);

    memcpy(result.data(), &typeVal, sizeof(typeVal));
    memcpy(result.data() + sizeof(typeVal), &flagsVal, sizeof(flagsVal));
    memcpy(result.data() + sizeof(typeVal) + sizeof(flagsVal), &lengthVal, sizeof(lengthVal));

    return result;
}

std::vector<char> serializeHeaderV(const TransportHeader& header) {
    std::array<char, TransportHeader::networkSize()> result = serializeHeaderA(header);
    return {result.begin(), result.end()};
}

static std::tuple<uint16_t, uint16_t, uint32_t> deserializeImpl(const std::span<const char> bytesData) {
    const char* dataPtr = bytesData.data();

    uint16_t typeValRaw;
    uint16_t flagsValRaw;
    uint32_t lengthValRaw;
    memcpy(&typeValRaw, dataPtr, sizeof(typeValRaw));
    memcpy(&flagsValRaw, dataPtr + sizeof(typeValRaw), sizeof(flagsValRaw));
    memcpy(&lengthValRaw, dataPtr + sizeof(typeValRaw) + sizeof(flagsValRaw), sizeof(lengthValRaw));

    return {networkToHost(typeValRaw), networkToHost(flagsValRaw), networkToHost(lengthValRaw)};
}

std::expected<TransportHeader, std::string> deserializeClientHeader(const std::span<const char> bytesData) {
    if (bytesData.size() < TransportHeader::networkSize()) {
        return std::unexpected("Insufficient data for TransportHeader");
    }

    const auto [typeValRaw, flagsValRaw, lengthValRaw] = deserializeImpl(bytesData);

    constexpr uint16_t maxTypeVal = maxTypeValue<ClientMessageType>();
    if (typeValRaw > maxTypeVal) {
        return std::unexpected("Invalid ClientMessageType");
    }

    return TransportHeader{static_cast<ClientMessageType>(typeValRaw), flagsValRaw, lengthValRaw};
}


std::expected<TransportHeader, std::string> deserializeServerHeader(const std::span<const char> bytesData) {
    if (bytesData.size() < TransportHeader::networkSize()) {
        return std::unexpected("Insufficient data for TransportHeader");
    }

    const auto [typeValRaw, flagsValRaw, lengthValRaw] = deserializeImpl(bytesData);

    constexpr uint16_t maxTypeVal = maxTypeValue<ServerMessageType>();
    if (typeValRaw > maxTypeVal) {
        return std::unexpected("Invalid ServerMessageType");
    }

    return TransportHeader{static_cast<ServerMessageType>(typeValRaw), flagsValRaw, lengthValRaw};

}
}