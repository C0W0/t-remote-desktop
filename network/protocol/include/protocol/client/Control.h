//
// Created by FNU Zhaluo on 10/14/26.
//

#ifndef TRDP_CONTROL_H
#define TRDP_CONTROL_H

#include <array>
#include <cstdint>

#include "protocol/Transport.h"

namespace network {
#define KEYBOARD_EVENTS(X)\
    X(KeyDown) \
    X(KeyUp) \

enum class ControlType : uint16_t {
    KEYBOARD_EVENTS(ENUM_ITEM)
    MouseLeftDown,
    MouseLeftUp,
    MouseRightDown,
    MouseRightUp,
    MouseMiddleDown,
    MouseMiddleUp,
    MouseMoved,
    MouseScrollDown,
    MouseScrollUp,
};

enum class KeyboardEvents : uint16_t {
    KEYBOARD_EVENTS(ENUM_ITEM)
};

#define CTRL_TYPE_FROM_KEYBOARD_EVENTS(name) \
    case KeyboardEvents::name: return ControlType::name; \

constexpr ControlType toControlType(KeyboardEvents event) {
    switch (event) {
        KEYBOARD_EVENTS(CTRL_TYPE_FROM_KEYBOARD_EVENTS)
    }
    std::unreachable();
}

struct ControlMeta {
    ControlType type;

    consteval static uint32_t networkSize() { return sizeof(ControlType); }
};

bool serializeControlMetaTo(ControlMeta meta, std::span<char> buffer);
std::expected<ControlMeta, std::string> deserializeControlMeta(std::span<char> buffer);

enum class KeyCode : uint16_t {
    SPACE = 0,
    ESCAPE,
    BACKSPACE,
    DELETE,
    ENTER,
    SHIFT,
    CONTROL,
    TAB,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, NINE, ZERO,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
};

// keyboard control packet format:
// | Header | ControlMeta (Keydown/KeyUp) | KeyCode |
struct KeyEventPayload {
    KeyCode key;

    consteval static uint32_t networkSize() { return sizeof(KeyCode); }
};

bool serializeKeyEventPayloadTo(KeyEventPayload payload, std::span<char> buffer);

std::array<char, TransportHeader::networkSize() + ControlMeta::networkSize() + KeyEventPayload::networkSize()>
serializeKeyEvent(KeyboardEvents type, KeyCode key);

// Mouse control packet format:
// | Header | ControlMeta (MouseDown/MouseUp/MouseScrolled) | Payload |
struct MouseEventPayload {
    uint16_t width;
    uint16_t height;
    uint16_t xPos;
    uint16_t yPos;
};

// Mouse moved packet format:
// | Header | ControlMeta (MouseMoved) | Payload |
struct MouseMovedEventPayload {
    uint16_t width;
    uint16_t height;
    uint16_t xFrom;
    uint16_t yFrom;
    uint16_t xTo;
    uint16_t yTo;
};
}

#endif //TRDP_CONTROL_H
