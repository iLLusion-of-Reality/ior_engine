#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

namespace ior_engine {
class core {
public:
    core() = default;
    ~core();

    bool init();
    void handle_event(const SDL_Event& event);
    void update();

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
};

} // namespace ior_engine