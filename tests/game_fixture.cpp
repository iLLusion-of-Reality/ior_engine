#include "game_api.h"

#include <exception>

#if defined(IOR_TEST_missing_entry)
extern "C" {
    int unrelated_symbol = 0;
}
#else
namespace {

void record(const ior_game_host* host, int event, float delta_time = 0.0f) {
    const auto probe = ecs_lookup(host->world, "LifecycleProbe");
    if (probe) {
        auto* transform = static_cast<ior_engine::local_transform*>(
            ecs_get_mut_id(host->world, probe, host->local_transform));
        transform->position[event] += 1.0f;
        const bool running = transform->rotation[0] == 1.0f;
        if ((event == 0 && running) || (event != 0 && !running)) {
            transform->rotation[1] = -1.0f;
        }
        if (event == 0 || event == 2) {
            transform->rotation[0] = event == 0 ? 1.0f : 0.0f;
        }
        transform->scale[0] = delta_time;
        transform->scale[1] = ecs_is_alive(host->world, host->root) ? 1.0f : -1.0f;
    }
}

bool init(const ior_game_host* host, void** context) noexcept {
    *context = new ior_game_host(*host);
    record(host, 0);
#if defined(IOR_TEST_lifecycle)
    return true;
#else
    // Fail after registering a system, so cleanup must remove callbacks before
    // unloading this image and restarting the previous game.
    ecs_entity_desc_t entity{};
    entity.parent = host->root;
    entity.name = "PartialSystem";
    ecs_system_desc_t system{};
    system.entity = ecs_entity_init(host->world, &entity);
    system.phase = EcsOnUpdate;
    system.callback = [](ecs_iter_t*) { std::terminate(); };
    system.ctx = new int(7);
    system.ctx_free = [](void* context) { delete static_cast<int*>(context); };
    ecs_system_init(host->world, &system);
    return false;
#endif
}

void update(void* context, float delta_time) noexcept {
    record(static_cast<ior_game_host*>(context), 1, delta_time);
}

void quit(void* context) noexcept {
    auto* host = static_cast<ior_game_host*>(context);
    record(host, 2);
    delete host;
}

} // namespace

IOR_GAME_EXPORT const ior_game_api* ior_game_query(uint32_t, uint32_t) noexcept {
    static const ior_game_api api{sizeof(ior_game_api),
#if defined(IOR_TEST_bad_abi)
        ior_game_abi + 1,
#else
        ior_game_abi,
#endif
        FLECS_VERSION_MAJOR, FLECS_VERSION_MINOR, FLECS_VERSION_PATCH,
#if defined(IOR_TEST_bad_layout)
        {},
#else
        ior_game_layout,
#endif
#if defined(IOR_TEST_bad_runtime)
        nullptr,
#else
        &ecs_get_world,
#endif
        init, update, quit};
    return &api;
}
#endif
