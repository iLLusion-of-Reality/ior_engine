#pragma once

#include <memory>

namespace flecs {
struct world;
}

namespace ior_engine {

class ecs {
public:
    ecs();
    ~ecs();

    ecs(const ecs&) = delete;
    ecs& operator=(const ecs&) = delete;

    void register_components();

    // A zero delta lets Flecs measure elapsed time itself.
    bool update(float delta_time = 0.0f);

    flecs::world& world();
    const flecs::world& world() const;

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace ior_engine
