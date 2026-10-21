#include "game_library.h"
#include "ecs.h"
#include "components.h"

#include <flecs.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void moving(ior_engine::ecs& ecs) {
    const auto player = ecs.world().lookup("::Game::Logic::Player");
    require(bool(player), "Player is missing");
    const float before = player.get<ior_engine::local_transform>().position[0];
    require(ecs.update(0.25f), "World stopped");
    const float after = player.get<ior_engine::local_transform>().position[0];
    require(std::abs(after - before - 0.25f) < 0.0001f, "Game system did not move exactly once");
}

void mapped_images(size_t expected) {
#if defined(__linux__)
    std::ifstream maps("/proc/self/maps");
    std::set<std::string> directories;
    for (std::string line; std::getline(maps, line);) {
        const auto start = line.find("/ior-game-");
        if (start != std::string::npos) {
            directories.insert(line.substr(start, line.find('/', start + 1) - start));
        }
    }
    require(directories.size() == expected, "OS loader retained a stale game image");
#endif
}

struct temporary_directory {
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        ("ior-reload-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    temporary_directory() { std::filesystem::create_directory(path); }
    ~temporary_directory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 8, "Expected game, five failure fixtures and lifecycle fixture");
        namespace fs = std::filesystem;
        ior_engine::ecs ecs;
        auto& world = ecs.world();
        const auto transform = world.id<ior_engine::local_transform>();
        const auto matrix = world.id<ior_engine::local_to_world>();
        const auto survivor = world.entity("HostEntity").set(ior_engine::local_transform{});
        ior_engine::game_library game(world);
        require(!game.load(argv[4]), "Initial partial failure was accepted");
        require(!game.loaded() && !world.lookup("::Game"), "Initial attach failure leaked its module");
        require(game.load(argv[1]), game.error());
        moving(ecs);
        const auto baseline = ecs_get_entities(world).alive_count;

        for (int i = 0; i < 64; ++i) {
            const auto old_root = world.lookup("::Game").id();
            require(game.reload(), game.error());
            require(!ecs_is_alive(world, old_root), "Old module root survived reload");
            moving(ecs);
            require(ecs_get_entities(world).alive_count == baseline, "Entities duplicated across reload");
            require(world.id<ior_engine::local_transform>() == transform, "Transform ID changed");
            require(world.id<ior_engine::local_to_world>() == matrix, "Matrix ID changed");
            require(survivor.is_alive(), "Host entity was removed");
            mapped_images(1);
        }

        for (int i : {2, 3, 5, 6}) {
            const auto root = world.lookup("::Game").id();
            require(!game.load(argv[i]), "Invalid library was accepted");
            require(world.lookup("::Game").id() == root, "Preflight failure disturbed active game");
            moving(ecs);
        }

        require(!game.load(argv[4]), "Partial attach failure was accepted");
        require(game.loaded(), "Previous game was not restarted");
        require(!world.lookup("::Game::PartialSystem"), "Failed candidate system survived");
        moving(ecs);

        temporary_directory directory;
        const auto watched = directory.path / fs::path(argv[1]).filename();
        fs::copy_file(argv[1], watched);
        require(game.load(watched), game.error());
        auto root = world.lookup("::Game").id();
        fs::last_write_time(watched, fs::last_write_time(watched) + std::chrono::seconds(1));
        require(game.poll(), game.error());
        require(world.lookup("::Game").id() == root, "Poll did not wait for stable file");
        std::this_thread::sleep_for(std::chrono::milliseconds(270));
        require(game.poll(), game.error());
        require(world.lookup("::Game").id() != root, "Poll did not reload changed file");
        moving(ecs);

        root = world.lookup("::Game").id();
        { std::ofstream broken(watched, std::ios::binary | std::ios::trunc); broken << "broken"; }
        std::this_thread::sleep_for(std::chrono::milliseconds(270));
        require(game.poll(), game.error());
        std::this_thread::sleep_for(std::chrono::milliseconds(270));
        require(!game.poll(), "Truncated library was accepted");
        require(world.lookup("::Game").id() == root, "Truncated build disturbed active game");
        moving(ecs);
        mapped_images(1);
        fs::copy_file(argv[1], watched, fs::copy_options::overwrite_existing);
        std::this_thread::sleep_for(std::chrono::milliseconds(270));
        require(game.poll(), game.error());
        std::this_thread::sleep_for(std::chrono::milliseconds(270));
        require(game.poll(), game.error());
        require(world.lookup("::Game").id() != root, "Poll did not recover after failed build");
        moving(ecs);

        bool rejected = false;
        std::thread other([&] { rejected = !game.unload(); });
        other.join();
        require(rejected && game.loaded(), "Wrong-thread unload was not rejected");
        ecs_readonly_begin(world, false);
        rejected = !game.reload();
        ecs_readonly_end(world);
        require(rejected, "Reload during readonly mode was not rejected");
        moving(ecs);

        struct frame_probe {
            ior_engine::game_library* game;
            bool rejected = false;
        } probe{&game};
        ecs_system_desc_t guard{};
        guard.entity = world.entity("ReloadGuardProbe").id();
        guard.phase = EcsOnUpdate;
        guard.immediate = true;
        guard.ctx = &probe;
        guard.callback = [](ecs_iter_t* iterator) {
            auto* value = static_cast<frame_probe*>(iterator->ctx);
            value->rejected = !value->game->reload();
        };
        const auto guard_id = ecs_system_init(world, &guard);
        require(guard_id != 0, "Cannot create frame guard probe");
        moving(ecs);
        require(probe.rejected, "Reload from an immediate system was not rejected");
        world.entity(guard_id).destruct();

        require(game.unload(), game.error());
        require(!world.lookup("::Game"), "Game subtree survived unload");
        mapped_images(0);
        require(survivor.is_alive(), "Host entity did not survive unload");
        require(ecs.update(0.25f), "World stopped after unload");
        require(game.load(argv[1]), game.error());
        moving(ecs);
        require(game.unload(), game.error());

        const auto probe_entity = world.entity("LifecycleProbe").set(ior_engine::local_transform{});
        auto lifecycle = [&] (float init, float update, float quit) {
            const auto& value = probe_entity.get<ior_engine::local_transform>();
            require(value.position[0] == init && value.position[1] == update &&
                value.position[2] == quit, "Lifecycle callback count/order mismatch");
            require(value.scale[1] == 1.0f, "Lifecycle callback ran after subtree deletion");
            require(value.rotation[1] == 0.0f, "Lifecycle callbacks ran out of order");
        };
        require(game.load(argv[7]), game.error());
        lifecycle(1, 0, 0);
        ecs.update(0.125f);
        lifecycle(1, 1, 0);
        require(probe_entity.get<ior_engine::local_transform>().scale[0] == 0.125f,
            "Game update received incorrect delta time");
        require(game.reload(), game.error());
        lifecycle(2, 1, 1);
        ecs.update(0.5f);
        lifecycle(2, 2, 1);
        require(game.unload(), game.error());
        ecs.update(0.25f);
        lifecycle(2, 2, 2);
        require(game.unload(), game.error());
        lifecycle(2, 2, 2);
        require(!game.load(argv[4]), "Failed init was accepted");
        lifecycle(3, 2, 3);
        require(!world.lookup("::Game"), "Failed init left a game subtree");
        {
            ior_engine::game_library scoped(world);
            require(scoped.load(argv[7]), scoped.error());
        }
        lifecycle(4, 2, 4);
        probe_entity.destruct();

        // A second world catches module type caches accidentally shared across
        // worlds or retained by the platform loader after dlclose.
        {
            ior_engine::ecs second;
            ior_engine::game_library module(second.world());
            require(module.load(argv[1]), module.error());
            moving(second);
        }
        std::cout << "GAME_RELOAD_OK cycles=64 host_ids=stable rollback=ok unload=ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "GAME_RELOAD_FAILED: " << error.what() << '\n';
        return 1;
    }
}
