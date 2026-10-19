//
// Created by Terry on 2026-07-21.
//

#ifndef TRDP_TRANSPORT_H
#define TRDP_TRANSPORT_H

#include <array>
#include <concepts>
#include <bit>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "magic_enum/magic_enum.hpp"

namespace network {
#define SHARED_MESSAGE_TYPES(X) \
    X(Message) \
    X(Disconnect) \

#define ENUM_ITEM(name) name,

enum class ClientMessageType : std::uint16_t {
    SHARED_MESSAGE_TYPES(ENUM_ITEM)
    Auth,
    Control,
};

enum class ServerMessageType : std::uint16_t {
    SHARED_MESSAGE_TYPES(ENUM_ITEM)
    Response,
    ImageFrame,
};

template<typename T>
requires (std::is_enum_v<T>)
constexpr std::string_view toString(T type) {
    return magic_enum::enum_name(type);
}

struct TransportHeader {
    uint16_t type;
    uint16_t flags;
    uint32_t length;

    constexpr TransportHeader(ClientMessageType type, uint16_t flags, uint32_t length) : type(std::to_underlying(type)), flags(flags), length(length) {}
    constexpr TransportHeader(ServerMessageType type, uint16_t flags, uint32_t length) : type(std::to_underlying(type)), flags(flags), length(length) {}

    [[nodiscard]] constexpr ClientMessageType getMessageTypeClient() const { return static_cast<ClientMessageType>(type); }
    [[nodiscard]] constexpr ServerMessageType getMessageTypeServer() const { return static_cast<ServerMessageType>(type); }

    consteval static uint32_t networkSize() { return 2*sizeof(uint16_t) + sizeof(uint32_t); }
};

std::array<char, TransportHeader::networkSize()> serializeHeaderA(const TransportHeader& header);
std::vector<char> serializeHeaderV(const TransportHeader& header);

std::expected<TransportHeader, std::string> deserializeClientHeader(std::span<const char> bytesData);
std::expected<TransportHeader, std::string> deserializeServerHeader(std::span<const char> bytesData);

template <typename T>
requires ((std::integral<T> && !std::same_as<T, bool>) || std::is_enum_v<T>)
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
requires ((std::integral<T> && !std::same_as<T, bool>) || std::is_enum_v<T>)
constexpr auto networkToHost(T value)
{
    return hostToNetwork(value); // should be identical; aliasing names to avoid confusion.
}
}

template <typename  T>
requires (std::is_enum_v<T>)
consteval auto maxTypeValue() {
    return std::to_underlying(magic_enum::enum_values<T>().back());
}


#endif //TRDP_TRANSPORT_H
