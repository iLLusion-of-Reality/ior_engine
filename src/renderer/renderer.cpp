#include "renderer.h"

#include <spdlog/spdlog.h>

namespace ior_engine {
    renderer::renderer(SDL_Window* window) : m_window(window) {
        m_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
        if(!m_device) {
            spdlog::critical("failed to create gpu device (vulkan)");
            throw std::runtime_error("gpu init failed");
        }

        if(!SDL_ClaimWindowForGPUDevice(m_device, window)) {
            spdlog::critical("failed to attach window for gpu device (vulkan)");
            SDL_DestroyGPUDevice(m_device);
            throw std::runtime_error("window claim failed");
        }

        SDL_SetGPUSwapchainParameters(m_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

        m_swapchain_format = SDL_GetGPUSwapchainTextureFormat(m_device, window);
        spdlog::info("successfull initialized gpu (vulkan)");
    }

    renderer::~renderer() {
        if(m_device) {
            if(m_window) {
                SDL_ReleaseWindowFromGPUDevice(m_device, m_window);
            }

            SDL_DestroyGPUDevice(m_device);
            spdlog::info("successfull device destroyed");
        }
    }

    void renderer::render() {
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_device);
        if(!cmd) return;

        SDL_GPUTexture* swapchain_tex = nullptr;
        Uint32 width, height;
        if(!SDL_AcquireGPUSwapchainTexture(cmd, m_window, &swapchain_tex, &width, &height)) {
            SDL_SubmitGPUCommandBuffer(cmd);
            return;
        }

        if(swapchain_tex) {
            SDL_GPUColorTargetInfo target_info{};
            target_info.texture = swapchain_tex;
            target_info.clear_color = SDL_FColor{ 20.0f / 255.0f, 20.0f / 255.0f, 35.0f / 255.0f, 1.0f };
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* renderpass = SDL_BeginGPURenderPass(cmd, &target_info, 1, nullptr);

            // TODO: rendering

            SDL_EndGPURenderPass(renderpass);
        }

        SDL_SubmitGPUCommandBuffer(cmd);
    }
}