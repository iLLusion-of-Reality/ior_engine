#pragma once
#include "ecs.h"

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

    ecs& get_ecs() { return m_ecs; }
    const ecs& get_ecs() const { return m_ecs; }

private:
    ecs m_ecs;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
};

} // namespace ior_engine
