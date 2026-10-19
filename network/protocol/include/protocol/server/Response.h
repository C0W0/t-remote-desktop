//
// Created by Terry Zha on 10/14/26.
//

#ifndef TRDP_RESPONSE_H
#define TRDP_RESPONSE_H

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace network {
enum class ResponseStatus : std::uint16_t {
    Ok,
    InvalidPassword,
    AccessDenied,
    UnsupportedVersion,
    ServerBusy,
    Error
};

// server response packet format:
// | Header | ServerRespMeta | message (optional) |
struct ServerRespMeta {
    ResponseStatus status;
    uint16_t messageLength;

    consteval static uint32_t networkSize() { return sizeof(ResponseStatus) + sizeof(uint16_t); }
};

std::array<char, ServerRespMeta::networkSize()> serializeServerRespMeta(const ServerRespMeta& meta);
bool serializeServerRespMetaAt(const ServerRespMeta& meta, std::span<char> buffer);
std::vector<char> serializeServerResponse(ResponseStatus status, std::span<const char> message);
std::expected<ServerRespMeta, std::string> deserializeServerRespMeta(std::span<const char> bytesData);
}

#endif //TRDP_RESPONSE_H
