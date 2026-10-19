//
// Created by Terry Zha on 10/14/26.
//

#ifndef TRDP_AUTH_H
#define TRDP_AUTH_H

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace network {
// authentication packet format:
// | Header | AuthMeta | password |
struct AuthMeta {
    uint16_t version;
    uint16_t passwordLength;

    consteval static uint32_t networkSize() { return 2*sizeof(uint16_t); }
};

std::expected<AuthMeta, std::string> deserializeAuthMeta(std::span<const char> bytesData);
}

#endif //TRDP_AUTH_H
