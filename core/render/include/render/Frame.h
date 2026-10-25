//
// Created by Terry Zha on 10/22/26.
//

#ifndef TRDP_FRAME_H
#define TRDP_FRAME_H

#include <SDL3/SDL.h>

namespace frame {
class SdlWindowWrapper {
public:
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

    virtual ~SdlWindowWrapper() {
        if (ptr_) {
            SDL_DestroyWindow(ptr_);
            ptr_ = nullptr;
        }
    }

    SDL_Window* operator->() { return ptr_; }

    virtual void renderFrame() = 0;
protected:
    explicit SdlWindowWrapper(SDL_Window* window): ptr_{window} {}

    SDL_Window* ptr_;
};
}

#endif //TRDP_FRAME_H
