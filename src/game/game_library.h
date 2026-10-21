#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace flecs { struct world; }

namespace ior_engine {

// Own on the world thread, and destroy before the world. Call poll only between
// progress calls, after all jobs using game callbacks have completed.
class game_library {
public:
    explicit game_library(flecs::world& world);
    ~game_library();

    game_library(const game_library&) = delete;
    game_library& operator=(const game_library&) = delete;

    bool load(const std::filesystem::path& source);
    bool reload();
    bool poll();
    bool unload();

    bool loaded() const;
    const std::string& error() const;

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace ior_engine
