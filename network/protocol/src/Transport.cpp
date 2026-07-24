//
// Created by Terry on 2026-07-21.
//

#include "protocol/Transport.h"

namespace network {
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

std::expected<TransportHeader, std::string> deserializeHeader(const std::span<const char> bytesData) {
    if (bytesData.size() < sizeof(TransportHeader)) {
        return std::unexpected("Insufficient data for TransportHeader");
    }

    const char* dataPtr = bytesData.data();

    uint16_t typeValRaw;
    uint16_t flagsValRaw;
    uint32_t lengthValRaw;
    memcpy(&typeValRaw, dataPtr, sizeof(typeValRaw));
    memcpy(&flagsValRaw, dataPtr + sizeof(typeValRaw), sizeof(flagsValRaw));
    memcpy(&lengthValRaw, dataPtr + sizeof(typeValRaw) + sizeof(flagsValRaw), sizeof(lengthValRaw));

    // uint16_t maxTypeVal = std::ranges::max(std::views::enum_range<MessageType>());
    //
    // if (typeValRaw > maxTypeVal) {
    //     return std::unexpected("Invalid MessageType");
    // }

    return TransportHeader {
        .type = static_cast<MessageType>(hostToNetwork(typeValRaw)),
        .flags = hostToNetwork(flagsValRaw),
        .length = hostToNetwork(lengthValRaw)
    };
}

std::expected<AuthMeta, std::string> deserializeAuthMeta(std::span<const char> bytesData) {
    if (bytesData.size() < sizeof(AuthMeta)) {
        return std::unexpected("Insufficient data for AuthMeta");
    }

    const char* dataPtr = bytesData.data();

    uint16_t versionRaw;
    uint16_t passwordLengthRaw;
    memcpy(&versionRaw, dataPtr, sizeof(versionRaw));
    memcpy(&passwordLengthRaw, dataPtr + sizeof(versionRaw), sizeof(passwordLengthRaw));

    return AuthMeta {
        .version = hostToNetwork(versionRaw),
        .passwordLength = hostToNetwork(passwordLengthRaw)
    };
}

std::array<char, sizeof(ServerRespMeta)> serializeServerRespMeta(const ServerRespMeta& meta) {
    std::array<char, sizeof(ServerRespMeta)> result{};
    const auto statusVal = hostToNetwork(meta.status);
    const auto lenVal = hostToNetwork(meta.messageLength);
    memcpy(result.data(), &statusVal, sizeof(statusVal));
    memcpy(result.data() + sizeof(statusVal), &lenVal, sizeof(lenVal));
    return result;
}

std::expected<ServerRespMeta, std::string> deserializeServerRespMeta(std::span<const char> bytesData) {
    if (bytesData.size() < sizeof(ServerRespMeta)) {
        return std::unexpected("Insufficient data for ServerRespMeta");
    }

    const char* dataPtr = bytesData.data();

    uint16_t statusRaw;
    uint16_t messageLengthRaw;
    memcpy(&statusRaw, dataPtr, sizeof(statusRaw));
    memcpy(&messageLengthRaw, dataPtr + sizeof(statusRaw), sizeof(messageLengthRaw));

    return ServerRespMeta {
        .status = static_cast<ResponseStatus>(hostToNetwork(statusRaw)),
        .messageLength = hostToNetwork(messageLengthRaw)
    };
}

std::array<char, sizeof(TransportHeader)> serializeDisconnect() {
    constexpr TransportHeader header {.type = MessageType::Disconnect, .flags = 0, .length = 0};
    return serializeHeaderA(header);
}
}