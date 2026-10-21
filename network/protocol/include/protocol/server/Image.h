//
// Created by Terry Zha on 10/14/26.
//

#ifndef TRDP_IMAGE_H
#define TRDP_IMAGE_H
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

namespace network {
enum class ImageType : uint8_t {
    JPG
};

struct FrameHeader {
    uint32_t magic;
    uint8_t version;
    ImageType type;
    uint16_t flags;
    uint32_t seq;
    uint32_t size;

    static consteval uint32_t networkSize() {
        return sizeof(uint32_t) + sizeof(uint8_t) + sizeof(ImageType) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
    }
};

bool serializeFrameHeaderInto(FrameHeader header, std::span<char> buffer);

std::expected<FrameHeader, std::string> deserializeFrameHeader(std::span<const char> bytesData);
}

#endif //TRDP_IMAGE_H
