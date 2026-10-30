//
// Created by Terry Zha on 10/30/26.
//

#ifndef TRDP_ENUMUTILS_H
#define TRDP_ENUMUTILS_H

#include <string_view>
#include <type_traits>
#include <utility>

#include "magic_enum/magic_enum.hpp"

template <typename T>
inline constexpr bool is_bit_operation_enabled_v = false;

template <typename T>
concept EnumWithBitOperation = std::is_enum_v<T> && is_bit_operation_enabled_v<T>;

template <EnumWithBitOperation T>
T operator|(T lhs, T rhs) {
    return static_cast<T>(
        std::to_underlying(lhs) | std::to_underlying(rhs)
    );
}

template <EnumWithBitOperation T>
T operator&(T lhs, T rhs) {
    return static_cast<T>(
        std::to_underlying(lhs) & std::to_underlying(rhs)
    );
}

template <EnumWithBitOperation T>
T operator^(T lhs, T rhs) {
    return static_cast<T>(
        std::to_underlying(lhs) ^ std::to_underlying(rhs)
    );
}

template <EnumWithBitOperation T>
T operator~(T rhs) {
    return static_cast<T>(~std::to_underlying(rhs));
}

template <EnumWithBitOperation T>
T& operator|=(T& lhs, T rhs) {
    return lhs = lhs | rhs;
}

template <EnumWithBitOperation T>
T& operator&=(T& lhs, T rhs) {
    return lhs = lhs & rhs;
}

template <EnumWithBitOperation T>
T& operator^=(T& lhs, T rhs) {
    return lhs = lhs ^ rhs;
}

namespace enum_utils {
template<typename T>
requires (std::is_enum_v<T>)
constexpr std::string_view toString(T type) {
    return magic_enum::enum_name(type);
}
}

#endif //TRDP_ENUMUTILS_H
