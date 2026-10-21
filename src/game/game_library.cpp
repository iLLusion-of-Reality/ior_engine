#include "game_library.h"
#include "game_api.h"
#include "components.h"

#include <SDL3/SDL_loadso.h>

#include <chrono>
#include <exception>
#include <random>
#include <thread>

namespace ior_engine {
namespace {

namespace fs = std::filesystem;
using clock = std::chrono::steady_clock;

struct file_stamp {
    fs::file_time_type time;
    uintmax_t size;
    bool operator==(const file_stamp&) const = default;
};

file_stamp stamp(const fs::path& path) {
    return {fs::last_write_time(path), fs::file_size(path)};
}

// Each candidate has a unique path: OS loader caching and build tools can never
// overwrite the currently executing image. RAII also cleans rejected images.
struct image {
    fs::path directory;
    SDL_SharedObject* handle = nullptr;
    ior_game_api api{};

    ~image() {
        if (handle) {
            SDL_UnloadObject(handle);
        }
        std::error_code ignored;
        fs::remove_all(directory, ignored);
    }
};

std::unique_ptr<image> open_image(const fs::path& source) {
    const auto before = stamp(source);
    if (!before.size) {
        throw std::runtime_error("Game library is empty");
    }

    auto result = std::make_unique<image>();
    std::random_device random;
    for (int attempt = 0; attempt < 16; ++attempt) {
        auto directory = fs::temp_directory_path() /
            ("ior-game-" + std::to_string(random()) + "-" + std::to_string(random()));
        if (fs::create_directory(directory)) {
            result->directory = directory;
            break;
        }
    }
    if (result->directory.empty()) {
        throw std::runtime_error("Cannot create private game library directory");
    }
    fs::permissions(result->directory, fs::perms::owner_all);
    const auto copy = result->directory / source.filename();
    fs::copy_file(source, copy);
    if (stamp(source) != before || fs::file_size(copy) != before.size) {
        throw std::runtime_error("Game library changed while copying; retry after build");
    }

    result->handle = SDL_LoadObject(copy.string().c_str());
    if (!result->handle) {
        throw std::runtime_error(SDL_GetError());
    }
    auto query = reinterpret_cast<ior_game_query_fn>(
        SDL_LoadFunction(result->handle, "ior_game_query"));
    if (!query) {
        throw std::runtime_error("Missing ior_game_query entry point");
    }
    const auto* api = query(ior_game_abi, sizeof(ior_game_host));
    if (!api || api->size != sizeof(ior_game_api) || api->abi != ior_game_abi ||
        api->flecs_major != FLECS_VERSION_MAJOR ||
        api->flecs_minor != FLECS_VERSION_MINOR ||
        api->flecs_patch != FLECS_VERSION_PATCH ||
        api->components != ior_game_layout || api->runtime != &ecs_get_world ||
        !api->init || !api->update || !api->quit) {
        throw std::runtime_error("Incompatible game SDK/Flecs ABI");
    }
    result->api = *api;
    return result;
}

} // namespace

struct game_library::impl {
    flecs::world& world;
    const std::thread::id owner = std::this_thread::get_id();
    std::unique_ptr<image> active;
    fs::path source;
    ecs_entity_t root = 0;
    ior_game_api* running_api = nullptr;
    void* context = nullptr;
    std::string error;
    file_stamp observed{};
    file_stamp attempted{};
    clock::time_point next_poll{};

    explicit impl(flecs::world& value) : world(value) {}

    bool ready() {
        if (std::this_thread::get_id() != owner) {
            error = "Game library must be used on its owning world thread";
            return false;
        }
        if (ecs_world_get_flags(world) &
            (EcsWorldReadonly | EcsWorldFrameInProgress | EcsWorldFini)) {
            error = "Game reload requires an idle, writable world between frames";
            return false;
        }
        return true;
    }

    void detach() {
        if (root) {
            const auto scope = ecs_set_scope(world, root);
            if (running_api) {
                running_api->quit(context);
                running_api = nullptr;
                context = nullptr;
            }
            ecs_set_scope(world, scope);
            // Deletes children, systems, queries, callback contexts, component
            // instances and type hooks BEFORE the image can be unloaded.
            world.entity(root).destruct();
            root = 0;
        }
    }

    bool start(image& library) {
        root = world.entity("::Game").add(flecs::Module).id();
        ior_game_host host{sizeof(ior_game_host), world, root,
            world.id<local_transform>(), world.id<local_to_world>()};
        // Imports may change scope. Always restore the host's scope.
        const auto scope = ecs_set_scope(world, root);
        running_api = &library.api;
        bool success = library.api.init(&host, &context);
        if (success) {
            ecs_system_desc_t system{};
            system.entity = world.entity("::Game::Update").id();
            system.phase = EcsOnUpdate;
            system.immediate = true;
            system.ctx = this;
            system.callback = [](ecs_iter_t* iterator) {
                auto* self = static_cast<impl*>(iterator->ctx);
                self->running_api->update(self->context, iterator->delta_time);
            };
            success = ecs_system_init(world, &system) != 0;
        }
        ecs_set_scope(world, scope);
        if (!success) {
            detach();
        }
        return success;
    }

    bool replace(const fs::path& path) {
        if (!ready()) {
            return false;
        }
        error.clear();
        try {
            if (!active && world.lookup("::Game")) {
                throw std::runtime_error("The Game namespace is already owned by another module");
            }
            const auto version = stamp(path);
            auto candidate = open_image(path);
            // Preflight is complete; old code remains loaded through deletion
            // and rollback. Invalid binaries never disturb the running game.
            detach();
            if (!start(*candidate)) {
                const bool restarting = bool(active);
                if (active && !start(*active)) {
                    active.reset();
                    throw std::runtime_error("Game init and previous module restart both failed");
                }
                throw std::runtime_error(restarting ?
                    "Game init failed; previous module restarted" : "Game init failed");
            }
            active = std::move(candidate);
            source = path;
            observed = attempted = version;
            return true;
        } catch (const std::exception& exception) {
            error = exception.what();
            return false;
        }
    }
};

game_library::game_library(flecs::world& world) : m_impl(std::make_unique<impl>(world)) {}

game_library::~game_library() {
    // A lifetime violation must never silently dlclose live Flecs callbacks.
    if (!unload()) {
        std::terminate();
    }
}

bool game_library::load(const fs::path& source) {
    try {
        return m_impl->replace(fs::canonical(source));
    } catch (const std::exception& exception) {
        m_impl->error = exception.what();
        return false;
    }
}

bool game_library::reload() {
    if (m_impl->source.empty()) {
        m_impl->error = "No game library has been loaded";
        return false;
    }
    return m_impl->replace(m_impl->source);
}

bool game_library::poll() {
    auto& state = *m_impl;
    if (!state.ready()) {
        return false;
    }
    if (!state.active || clock::now() < state.next_poll) {
        return true;
    }
    state.next_poll = clock::now() + std::chrono::milliseconds(250);
    try {
        const auto current = stamp(state.source);
        if (current != state.observed) {
            state.observed = current;
        } else if (current != state.attempted) {
            state.attempted = current;
            return state.replace(state.source);
        }
        return true;
    } catch (const std::exception& exception) {
        state.error = exception.what();
        return false;
    }
}

bool game_library::unload() {
    if (!m_impl->ready()) {
        return false;
    }
    m_impl->detach();
    m_impl->active.reset();
    m_impl->source.clear();
    m_impl->error.clear();
    return true;
}

bool game_library::loaded() const { return bool(m_impl->active); }
const std::string& game_library::error() const { return m_impl->error; }

} // namespace ior_engine
