//
// Created by Terry Zha on 10/22/26.
//

#include "metal/MetalFrame.h"

#include "render/SdlContextInitializer.h"

#include <print>

using namespace frame;
using namespace renderer;

int main() {
    SdlContextInitializer::Initialize();

    auto createResult = MetalWindow::Create(
        "TRDP - Render Demo",
        1280,
        720,
        SDL_WINDOW_RESIZABLE
    );
    if (!createResult) {
        std::println("{}", createResult.error());
        return 1;
    }

    MetalWindow window = std::move(createResult).value();

    bool running = true;
    SDL_Event event{};
    while (running) {
        if (SDL_WaitEventTimeout(&event, 1000/60)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
                default:
                    break;
            }
        }
        window.renderFrame();
    }

    return 0;
}