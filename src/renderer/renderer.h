#pragma once

#include <SDL3/SDL.h>

namespace ior_engine {
class renderer {
public:
    renderer(SDL_Window* window);
    ~renderer();

    renderer(const renderer&) = delete;
    renderer& operator=(const renderer&) = delete;

    void render();
private:
    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_swapchain_format = SDL_GPU_TEXTUREFORMAT_INVALID;
};

} // ior_engine