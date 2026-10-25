//
// Created by FNU Zhaluo on 10/23/26.
//

#ifndef TRDP_METALFRAME_H
#define TRDP_METALFRAME_H

#include <memory>
#include <print>
#include <expected>
#include <string>

#include <SDL3/SDL.h>

#include "render/Frame.h"

namespace frame {
class MetalWindow;
}

namespace renderer {
class MetalRenderer {
public:
    MetalRenderer(const MetalRenderer&) = delete;
    MetalRenderer& operator=(const MetalRenderer&) = delete;
    MetalRenderer(MetalRenderer&&) noexcept;
    MetalRenderer& operator=(MetalRenderer&&) noexcept;

    ~MetalRenderer();

    static std::expected<MetalRenderer, std::string> Create(frame::MetalWindow& window);
    void renderFrame();
private:
    friend class frame::MetalWindow;

    MetalRenderer();

    class Impl;
    std::unique_ptr<Impl> impl_;
};
}

namespace frame {
class MetalWindow final : public SdlWindowWrapper {
public:
    static std::expected<MetalWindow, std::string> Create(const char* title, int w, int h, SDL_WindowFlags flags);

    MetalWindow(const MetalWindow&) = delete;
    MetalWindow& operator=(const MetalWindow&) = delete;
    MetalWindow(MetalWindow&& other) noexcept :
        SdlWindowWrapper{other.ptr_}, metalView_ {other.metalView_}, renderer_{std::move(other.renderer_)}
    {
        other.ptr_ = nullptr;
        other.metalView_ = nullptr;
    }
    MetalWindow& operator=(MetalWindow&& other) noexcept {
        if (this != &other) {
            ptr_ = other.ptr_;
            metalView_ = other.metalView_;
            renderer_ = std::move(other.renderer_);
            other.ptr_ = nullptr;
            other.metalView_ = nullptr;
        }
        return *this;
    }

    ~MetalWindow() override {
        if (metalView_) {
            SDL_Metal_DestroyView(metalView_);
        }
    }

    [[nodiscard]] SDL_MetalView metalView() const noexcept {
        return metalView_;
    }

    void renderFrame() override {
        renderer_.renderFrame();
    }

private:
    explicit MetalWindow(SDL_Window* window, SDL_MetalView view) : SdlWindowWrapper{window}, metalView_{view} {}

    SDL_MetalView metalView_;
    renderer::MetalRenderer renderer_;
};
}


#endif //TRDP_METALFRAME_H
