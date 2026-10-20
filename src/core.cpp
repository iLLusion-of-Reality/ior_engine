#define SDL_MAIN_USE_CALLBACKS 1
#include "core.h"

#include <spdlog/spdlog.h>

namespace ior_engine {
    core::~core() {
        if (m_renderer) {
            SDL_DestroyRenderer(m_renderer);
        }
        if (m_window) {
            SDL_DestroyWindow(m_window);
        }
        
        spdlog::info("Core destroyed.");
    }

    bool core::init() {
        if(!SDL_CreateWindowAndRenderer("IOR Engine", 800, 600, SDL_WINDOW_RESIZABLE, &m_window, &m_renderer)) {
            spdlog::critical("Failed to create window and renderer: %s", SDL_GetError());
            return false;
        }
        return true;
    }

    void core::handle_event(const SDL_Event& event) {
        if(event.type == SDL_EVENT_KEY_DOWN) {
            if(event.key.key == SDLK_ESCAPE) {
                spdlog::info("Escape key pressed. Exiting...");
                SDL_Quit();
                exit(0);
            }
        }
    }

    void core::update() {
        SDL_SetRenderDrawColor(m_renderer, 20, 20, 35, 255);
        SDL_RenderClear(m_renderer);
        SDL_RenderPresent(m_renderer);
    }
} // namespace ior_engine

// SDL3 CALLBACKS
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    auto core = std::make_unique<ior_engine::core>();

    if (!core->init()) {
        return SDL_APP_FAILURE;
    }

    *appstate = core.release(); 
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    ior_engine::core* core = static_cast<ior_engine::core*>(appstate);

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    core->handle_event(*event); 
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate) {
    ior_engine::core* core = static_cast<ior_engine::core*>(appstate);
    
    core->update();
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (appstate) {
        ior_engine::core* core = static_cast<ior_engine::core*>(appstate);
        delete core;
    }
    SDL_Quit();
}