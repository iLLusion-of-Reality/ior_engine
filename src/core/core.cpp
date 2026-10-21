#define SDL_MAIN_USE_CALLBACKS 1
#include "core.h"

#include <spdlog/spdlog.h>

namespace ior_engine {
    core::~core() {
        m_game.unload();

        m_renderer.reset();

        if (m_window) {
            SDL_DestroyWindow(m_window);
        }
        
        spdlog::info("core destroyed.");
    }

    bool core::init(const std::filesystem::path& game_path) {
        m_window = SDL_CreateWindow("IOR Engine", 800, 600, SDL_WINDOW_RESIZABLE);
        if(!m_window) {
            spdlog::critical("failed to create window: {}", SDL_GetError());
            return false;
        }
        
        m_renderer = std::make_unique<renderer>(m_window);

        if (!game_path.empty() && !m_game.load(game_path)) {
            spdlog::critical("game load failed: {}", m_game.error());
            return false;
        }
        if (m_game.loaded()) {
            spdlog::info("game loaded: {} (hot reload enabled)", game_path.string());
        }
        return true;
    }

    void core::handle_event(const SDL_Event& event) {
        if(event.type == SDL_EVENT_KEY_DOWN) {
            if(event.key.key == SDLK_ESCAPE) {
                spdlog::info("escape key pressed. exiting...");
                SDL_Event quit{};
                quit.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit);
            }
        }
    }

    void core::update() {
        if (!m_game.poll()) {
            if (m_reload_error != m_game.error()) {
                m_reload_error = m_game.error();
                spdlog::error("game reload failed: {}", m_reload_error);
            }
        } else if (m_game.error().empty()) {
            m_reload_error.clear();
        }
        m_ecs.update();

        m_renderer->render();
        // SDL_SetRenderDrawColor(m_renderer, 20, 20, 35, 255);
        // SDL_RenderClear(m_renderer);
        // SDL_RenderPresent(m_renderer);
    }
} // namespace ior_engine

// SDL3 CALLBACKS
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    std::filesystem::path game_path;
    if (argc == 3 && std::string_view(argv[1]) == "--game") {
        game_path = argv[2];
    } else if (argc != 1) {
        spdlog::error("Usage: ior_engine [--game /absolute/path/to/game3 library]");
        return SDL_APP_FAILURE;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::critical("SDL video initialization failed: {}", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto core = std::make_unique<ior_engine::core>();

    if (!core->init(game_path)) {
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
