//
// Created by Terry Zha on 10/22/26.
//

#ifndef TRDP_FRAME_H
#define TRDP_FRAME_H

#include <print>
#include <expected>
#include <string>
#include <SDL3/SDL.h>

namespace frame {
class SdlWindowWrapper {
public:
    static std::expected<SdlWindowWrapper, std::string> Create(const char* title, int w, int h, SDL_WindowFlags flags)  {
        SDL_Window* window = SDL_CreateWindow(title, w, h, flags);
        if (!window) {
            return std::unexpected{std::format("SDL_CreateWindow failed: {}", SDL_GetError())};
        }
        return SdlWindowWrapper(window);
    }

    SdlWindowWrapper(const SdlWindowWrapper&) = delete;
    SdlWindowWrapper& operator=(const SdlWindowWrapper&) = delete;
    SdlWindowWrapper(SdlWindowWrapper&& other) noexcept {
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
    }
    SdlWindowWrapper& operator=(SdlWindowWrapper&& other)  noexcept {
        if (this == &other) {
            return *this;
        }
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        return *this;
    }

    ~SdlWindowWrapper() {
        if (ptr_) {
            SDL_DestroyWindow(ptr_);
            ptr_ = nullptr;
        }
    }

    SDL_Window* operator->() { return ptr_; }
private:
    explicit SdlWindowWrapper(SDL_Window* window): ptr_{window} {}

    SDL_Window* ptr_;
};
}

#endif //TRDP_FRAME_H
