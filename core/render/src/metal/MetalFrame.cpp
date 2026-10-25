//
// Created by FNU Zhaluo on 10/23/26.
//

#include "MetalFrame.h"

using namespace frame;

std::expected<MetalWindow, std::string>
MetalWindow::Create(const char* title, int w, int h, SDL_WindowFlags flags) {
    SDL_Window* window = SDL_CreateWindow(title, w, h, flags | SDL_WINDOW_METAL);
    if (!window) {
        return std::unexpected{std::format("SDL_CreateWindow failed: {}", SDL_GetError())};
    }
    SDL_MetalView view = SDL_Metal_CreateView(window);
    if (!view) {
        return std::unexpected{std::format("SDL_CreateView failed: {}", SDL_GetError())};
    }

    MetalWindow metalWindow{window, view};

    auto rendererCreateResult = renderer::MetalRenderer::Create(metalWindow);
    if (!rendererCreateResult) {
        return std::unexpected{std::format("MetalRenderer creation failed: {}", rendererCreateResult.error())};
    }
    metalWindow.renderer_ = std::move(rendererCreateResult).value();

    return metalWindow;
}
