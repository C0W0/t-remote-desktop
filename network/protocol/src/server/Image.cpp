#include "protocol/server/Image.h"

#include <cstring>

#include "protocol/Transport.h"

namespace network {
bool serializeFrameHeaderInto(FrameHeader header, std::span<char> buffer) {
    if (buffer.size() < FrameHeader::networkSize()) {
        return false;
    }

    const auto magicVal = hostToNetwork(header.magic);
    const auto versionVal = hostToNetwork(header.version);
    const auto typeVal = hostToNetwork(header.type);
    const auto flagsVal = hostToNetwork(header.flags);
    const auto seqVal = hostToNetwork(header.seq);
    const auto sizeVal = hostToNetwork(header.size);

    memcpy(buffer.data(), &magicVal, sizeof(magicVal));
    buffer = buffer.subspan(sizeof(magicVal));
    memcpy(buffer.data(), &versionVal, sizeof(versionVal));
    buffer = buffer.subspan(sizeof(versionVal));
    memcpy(buffer.data(), &typeVal, sizeof(typeVal));
    buffer = buffer.subspan(sizeof(typeVal));
    memcpy(buffer.data(), &flagsVal, sizeof(flagsVal));
    buffer = buffer.subspan(sizeof(flagsVal));
    memcpy(buffer.data(), &seqVal, sizeof(seqVal));
    buffer = buffer.subspan(sizeof(seqVal));
    memcpy(buffer.data(), &sizeVal, sizeof(sizeVal));
    return true;
}

std::expected<FrameHeader, std::string> deserializeFrameHeader(std::span<const char> bytesData) {
    if (bytesData.size() < FrameHeader::networkSize()) {
        return std::unexpected{"Insufficient data for frame header"};
    }

    uint32_t magicRaw;
    uint8_t versionRaw;
    uint8_t typeRaw;
    uint16_t flagsRaw;
    uint32_t seqRaw;
    uint32_t sizeRaw;

    memcpy(&magicRaw, bytesData.data(), sizeof(magicRaw));
    bytesData = bytesData.subspan(sizeof(magicRaw));
    memcpy(&versionRaw, bytesData.data(), sizeof(versionRaw));
    bytesData = bytesData.subspan(sizeof(versionRaw));

    memcpy(&typeRaw, bytesData.data(), sizeof(typeRaw));
    bytesData = bytesData.subspan(sizeof(typeRaw));
    if (networkToHost(typeRaw) > maxTypeValue<ImageType>()) {
        return std::unexpected("Invalid ResponseStatus");
    }

    memcpy(&flagsRaw, bytesData.data(), sizeof(flagsRaw));
    bytesData = bytesData.subspan(sizeof(flagsRaw));
    memcpy(&seqRaw, bytesData.data(), sizeof(seqRaw));
    bytesData = bytesData.subspan(sizeof(seqRaw));
    memcpy(&sizeRaw, bytesData.data(), sizeof(sizeRaw));

    return FrameHeader{
        .magic = networkToHost(magicRaw),
        .version = networkToHost(versionRaw),
        .type = static_cast<ImageType>(networkToHost(typeRaw)),
        .flags = networkToHost(flagsRaw),
        .seq = networkToHost(seqRaw),
        .size = networkToHost(sizeRaw)
    };
}
}
