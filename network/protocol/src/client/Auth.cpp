#include "protocol/client/Auth.h"
#include "protocol/Transport.h"

#include <cstring>

namespace network {
std::expected<AuthMeta, std::string> deserializeAuthMeta(std::span<const char> bytesData) {
    if (bytesData.size() < AuthMeta::networkSize()) {
        return std::unexpected("Insufficient data for AuthMeta");
    }

    const char* dataPtr = bytesData.data();

    uint16_t versionRaw;
    uint16_t passwordLengthRaw;
    memcpy(&versionRaw, dataPtr, sizeof(versionRaw));
    memcpy(&passwordLengthRaw, dataPtr + sizeof(versionRaw), sizeof(passwordLengthRaw));

    return AuthMeta {
        .version = networkToHost(versionRaw),
        .passwordLength = networkToHost(passwordLengthRaw)
    };
}
}