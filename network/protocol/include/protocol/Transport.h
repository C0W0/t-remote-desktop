//
// Created by Terry on 2026-07-21.
//

#ifndef TRDP_TRANSPORT_H
#define TRDP_TRANSPORT_H

#include <array>
#include <concepts>
#include <bit>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace network {

enum class MessageType : std::uint16_t {
    Auth,
    ServerResp,
    Message,
    Disconnect
};

constexpr const char* toString(MessageType type) {
    switch (type) {
        case MessageType::Auth:
            return "Auth";
        case MessageType::ServerResp:
            return "ServerResp";
        case MessageType::Message:
            return "Message";
        case MessageType::Disconnect:
            return "Disconnect";
    }
    std::unreachable();
}

struct TransportHeader {
    MessageType type;
    uint16_t flags;
    uint32_t length;
};

std::array<char, sizeof(TransportHeader)> serializeHeaderA(const TransportHeader& header);
std::vector<char> serializeHeaderV(const TransportHeader& header);

std::expected<TransportHeader, std::string> deserializeHeader(std::span<const char> bytesData);

struct AuthMeta {
    uint16_t version;
    uint16_t passwordLength;
};

enum class ResponseStatus : std::uint16_t {
    Ok,
    InvalidPassword,
    AccessDenied,
    UnsupportedVersion,
    ServerBusy,
    Error
};

std::expected<AuthMeta, std::string> deserializeAuthMeta(std::span<const char> bytesData);

constexpr const char* toString(ResponseStatus status) {
    switch (status) {
        case ResponseStatus::Ok:
            return "Ok";
        case ResponseStatus::InvalidPassword:
            return "InvalidPassword";
        case ResponseStatus::AccessDenied:
            return "AccessDenied";
        case ResponseStatus::UnsupportedVersion:
            return "UnsupportedVersion";
        case ResponseStatus::ServerBusy:
            return "ServerBusy";
        case ResponseStatus::Error:
            return "Error";
    }
    std::unreachable();
}

struct ServerRespMeta {
    ResponseStatus status;
    uint16_t messageLength;
};

std::array<char, sizeof(ServerRespMeta)> serializeServerRespMeta(const ServerRespMeta& meta);
std::expected<ServerRespMeta, std::string> deserializeServerRespMeta(std::span<const char> bytesData);

template <typename T>
requires (std::integral<T> && !std::same_as<T, bool>) || std::is_enum_v<T>
constexpr auto hostToNetwork(T value)
{
    if constexpr (std::is_enum_v<T>) {
        return hostToNetwork(std::to_underlying(value));
    } else {
        if constexpr (std::endian::native == std::endian::little) {
            return std::byteswap(value);
        } else {
            return value;
        }
    }
}


template <typename T>
requires (std::integral<T> && !std::same_as<T, bool>) || std::is_enum_v<T>
constexpr auto networkToHost(T value)
{
    return hostToNetwork(value); // should be identical; aliasing names to avoid confusion.
}

std::array<char, sizeof(TransportHeader)> serializeDisconnect();

}

#endif //TRDP_TRANSPORT_H
