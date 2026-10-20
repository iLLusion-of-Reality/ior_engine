#include "ecs.h"

#include <flecs.h>

namespace ior_engine {

struct ecs::impl {
    flecs::world world;
};

ecs::ecs() : m_impl(std::make_unique<impl>()) {}

ecs::~ecs() = default;

bool ecs::update(float delta_time) {
    return m_impl->world.progress(delta_time);
}

flecs::world& ecs::world() {
    return m_impl->world;
}

const flecs::world& ecs::world() const {
    return m_impl->world;
}

} // namespace ior_engine
