#include "ecs.h"
#include "components.h"

#include <flecs.h>

namespace ior_engine {

struct ecs::impl {
    flecs::world world;
};

ecs::ecs() : m_impl(std::make_unique<impl>()) {
    register_components();
}

ecs::~ecs() = default;

void ecs::register_components() {
    m_impl->world.component<local_transform>();
    m_impl->world.component<local_to_world>();
}

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
