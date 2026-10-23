//
// Created by Terry Zha on 10/23/26.
//

#ifndef TRDP_SDLCONTEXTINITIALIZER_H
#define TRDP_SDLCONTEXTINITIALIZER_H

#include <print>
#include <SDL3/SDL.h>

namespace frame {
class SdlContextInitializer {
public:
    static void Initialize() {
        static SdlContextInitializer instance;
    }

    SdlContextInitializer(const SdlContextInitializer&) = delete;
    SdlContextInitializer& operator=(const SdlContextInitializer&) = delete;
private:
    SdlContextInitializer() {
        std::println("SDL initialized");
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::format("SDL_Init failed: {}", SDL_GetError()));
        }
    }

    ~SdlContextInitializer() {
        std::println("SDL destroyed");
        SDL_Quit();
    }
};
}

#endif //TRDP_SDLCONTEXTINITIALIZER_H
